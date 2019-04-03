#include "myssh.h"

ssh_session ssh_ptr_get(SEXP ptr){
  ssh_session ssh = (ssh_session) R_ExternalPtrAddr(ptr);
  if(ssh == NULL)
    Rf_error("SSH session pointer is dead");
  if(!ssh_is_connected(ssh))
    Rf_error("ssh session has been disconnected");
  return ssh;
}

static void ssh_ptr_fin(SEXP ptr){
  ssh_session ssh = (ssh_session) R_ExternalPtrAddr(ptr);
  if(ssh == NULL)
    return;
  if(ssh_is_connected(ssh)){
    Rf_warningcall(R_NilValue, "Disconnecting from unused ssh session. Please use ssh_disconnect()\n");
    ssh_disconnect(ssh);
  }
  ssh_free(ssh);
  R_ClearExternalPtr(ptr);
}

static SEXP ssh_ptr_create(ssh_session ssh){
  SEXP ptr = PROTECT(R_MakeExternalPtr(ssh, R_NilValue, R_NilValue));
  R_RegisterCFinalizerEx(ptr, ssh_ptr_fin, TRUE);
  Rf_setAttrib(ptr, R_ClassSymbol, Rf_mkString("ssh_session"));
  UNPROTECT(1);
  return ptr;
}

static void assert_or_disconnect(int rc, const char * what, ssh_session ssh){
  if (rc != SSH_OK){
    char buf[1024];
    strncpy(buf, ssh_get_error(ssh), 1023);
    ssh_disconnect(ssh);
    Rf_errorcall(R_NilValue, "libssh failure at '%s': %s", what, buf);
  }
}

static int password_cb(SEXP rpass, const char * prompt, char *buf, int buflen, const char *user){
  if(Rf_isString(rpass) && Rf_length(rpass)){
    strncpy(buf, CHAR(STRING_ELT(rpass, 0)), buflen);
    return SSH_OK;
  } else if(Rf_isFunction(rpass)){
    int err;
    if(strcmp(prompt, "Passphrase") == 0) //nicer wording
      prompt = "Passphrase for reading private key";
    SEXP question = PROTECT(make_string(prompt));
    Rf_setAttrib(question, R_NamesSymbol, make_string(user));
    SEXP call = PROTECT(Rf_lcons(rpass, Rf_lcons(question, R_NilValue)));
    SEXP res = PROTECT(R_tryEval(call, R_GlobalEnv, &err));
    if(err || !Rf_isString(res)){
      UNPROTECT(3);
      REprintf("Password callback did not return a string value\n");
      return SSH_ERROR;
    }
    strncpy(buf, CHAR(STRING_ELT(res, 0)), buflen);
    UNPROTECT(3);
    return SSH_OK;
  }
  REprintf("unsupported password type\n");
  return SSH_ERROR;
}

static int my_auth_callback(const char *prompt, char *buf, size_t len, int echo, int verify, void *rpass){
  return password_cb((SEXP) rpass, prompt, buf, len, "");
}

static int auth_password(ssh_session ssh, SEXP rpass, const char *user){
  char buf[1024];
  char prompt[1024];
  snprintf(prompt, 1023, "Please enter ssh password for user '%s'", user ? user : "???");
  return password_cb(rpass, prompt, buf, 1024, user) || ssh_userauth_password(ssh, NULL, buf);
}

static int auth_interactive(ssh_session ssh, SEXP rpass, const char *user){
  int rc = ssh_userauth_kbdint(ssh, NULL, NULL);
  while (rc == SSH_AUTH_INFO) {
    const char * name = ssh_userauth_kbdint_getname(ssh);
    const char * instruction = ssh_userauth_kbdint_getinstruction(ssh);
    int nprompts = ssh_userauth_kbdint_getnprompts(ssh);
    if (strlen(name) > 0)
      Rprintf("%s\n", name);
    if (strlen(instruction) > 0)
      Rprintf("%s\n", instruction);
    for (int iprompt = 0; iprompt < nprompts; iprompt++) {
      char buf[1024] = {0};
      const char * prompt = ssh_userauth_kbdint_getprompt(ssh, iprompt, NULL);
      char question[1024];
      snprintf(question, 1023, "Authenticating user '%s'. %s", user, prompt);
      password_cb(rpass, question, buf, 1024, user);
      if (ssh_userauth_kbdint_setanswer(ssh, iprompt, buf) < 0)
        return SSH_AUTH_ERROR;
    }
    rc = ssh_userauth_kbdint(ssh, NULL, NULL);
  }
  return rc;
}

/* authenticate client */
static void auth_or_disconnect(ssh_session ssh, ssh_key privkey, SEXP rpass, const char *user){
  if(ssh_userauth_none(ssh, NULL) == SSH_AUTH_SUCCESS)
    return;
  int method = ssh_userauth_list(ssh, NULL);
  if (method & SSH_AUTH_METHOD_PUBLICKEY){
    if(privkey != NULL && ssh_userauth_publickey(ssh, NULL, privkey) == SSH_AUTH_SUCCESS)
      return;
    // ssh_userauth_publickey_auto() tries both ssh-agent and standard keys in ~/.ssh
    // it also automatically picks up SSH_ASKPASS env var set by 'askpass' package
    if(privkey == NULL && ssh_userauth_publickey_auto(ssh, NULL, NULL) == SSH_AUTH_SUCCESS)
      return;
  }
  if (method & SSH_AUTH_METHOD_INTERACTIVE && auth_interactive(ssh, rpass, user) == SSH_AUTH_SUCCESS)
    return;
  if (method & SSH_AUTH_METHOD_PASSWORD && auth_password(ssh, rpass, user) == SSH_AUTH_SUCCESS)
    return;
  ssh_disconnect(ssh);
  Rf_errorcall(R_NilValue, "Authentication with ssh server failed");
}

SEXP C_start_session(SEXP rhost, SEXP rport, SEXP ruser, SEXP keyfile, SEXP rpass, SEXP verbosity){

  /* try reading private key first */
  ssh_key privkey = NULL;
  if(Rf_length(keyfile))
    if(ssh_pki_import_privkey_file(CHAR(STRING_ELT(keyfile, 0)), NULL, my_auth_callback, rpass, &privkey) != SSH_OK)
      Rf_error("Failed to read private key: %s", CHAR(STRING_ELT(keyfile, 0)));

  /* load options */
  int loglevel = Rf_asInteger(verbosity);
  int port = Rf_asInteger(rport);
  const char * host = CHAR(STRING_ELT(rhost, 0));
  const char * user = CHAR(STRING_ELT(ruser, 0));
  ssh_session ssh = ssh_new();
  SEXP ptr = PROTECT(ssh_ptr_create(ssh));
  assert_or_disconnect(ssh_options_set(ssh, SSH_OPTIONS_HOST, host), "set host", ssh);
  assert_or_disconnect(ssh_options_set(ssh, SSH_OPTIONS_USER, user), "set user", ssh);
  assert_or_disconnect(ssh_options_set(ssh, SSH_OPTIONS_PORT, &port), "set port", ssh);
  assert_or_disconnect(ssh_options_set(ssh, SSH_OPTIONS_LOG_VERBOSITY, &loglevel), "set verbosity", ssh);

  /* sets password callback for default private key */
  struct ssh_callbacks_struct cb = {
    .userdata = rpass,
    .auth_function = my_auth_callback
  };
  ssh_callbacks_init(&cb);
  ssh_set_callbacks(ssh, &cb);

  /* connect */
  assert_or_disconnect(ssh_connect(ssh), "connect", ssh);

  /* get server identity */
  ssh_key key;
  unsigned char * hash = NULL;
  size_t hlen = 0;
  assert_or_disconnect(myssh_get_publickey(ssh, &key), "myssh_get_publickey", ssh);
  assert_or_disconnect(ssh_get_publickey_hash(key, SSH_PUBLICKEY_HASH_SHA1, &hash, &hlen), "ssh_get_publickey_hash", ssh);
#if LIBSSH_VERSION_MINOR < 8
  if(!ssh_is_server_known(ssh)){
    Rprintf("Server fingerprint: %s\n", ssh_get_hexa(hash, hlen));
  }
#else
  switch(ssh_session_is_known_server(ssh)){
  case SSH_KNOWN_HOSTS_OK:
    if(loglevel > 0)
      REprintf("Found known server key: %s\n", ssh_get_hexa(hash, hlen));
    break;
  case SSH_KNOWN_HOSTS_UNKNOWN:
    REprintf("New server key: %s\n", ssh_get_hexa(hash, hlen));
    ssh_session_update_known_hosts(ssh);
    break;
  case SSH_KNOWN_HOSTS_OTHER:
  case SSH_KNOWN_HOSTS_CHANGED:
    REprintf("Server key has changed (possible attack?!): %s\nPlease update your ~/.ssh/known_hosts file\n", ssh_get_hexa(hash, hlen));
    break;
  case SSH_KNOWN_HOSTS_ERROR:
  case SSH_KNOWN_HOSTS_NOT_FOUND:
    if(loglevel > 0)
      REprintf("Could not load a known_hosts file\n");
    break;
  }
#endif

  /* Authenticate client or error */
  auth_or_disconnect(ssh, privkey, rpass, user);

  /* display welcome message */
  char * banner = ssh_get_issue_banner(ssh);
  if(banner != NULL){
    Rprintf("%s\n", banner);
    free(banner);
  }

  UNPROTECT(1);
  return ptr;
}

SEXP C_ssh_info(SEXP ptr){
  ssh_session ssh = ssh_ptr_get(ptr);
  unsigned int port;
  char * user = NULL;
  char * host = NULL;
  char * identity = NULL;
  int connected = ssh_is_connected(ssh);
  ssh_options_get_port(ssh, &port);
  ssh_options_get(ssh, SSH_OPTIONS_USER, &user);
  ssh_options_get(ssh, SSH_OPTIONS_HOST, &host);
  ssh_options_get(ssh, SSH_OPTIONS_IDENTITY, &identity);


  ssh_key key;
  unsigned char * hash = NULL;
  size_t hlen = 0;
  assert_or_disconnect(myssh_get_publickey(ssh, &key), "ssh_get_publickey", ssh);
  assert_or_disconnect(ssh_get_publickey_hash(key, SSH_PUBLICKEY_HASH_SHA1, &hash, &hlen), "ssh_get_publickey_hash", ssh);

  SEXP out = PROTECT(Rf_allocVector(VECSXP, 6));
  SET_VECTOR_ELT(out, 0, make_string(user));
  SET_VECTOR_ELT(out, 1, make_string(host));
  SET_VECTOR_ELT(out, 2, make_string(identity));
  SET_VECTOR_ELT(out, 3, Rf_ScalarInteger(port));
  SET_VECTOR_ELT(out, 4, Rf_ScalarLogical(connected));
  SET_VECTOR_ELT(out, 5, make_string(ssh_get_hexa(hash, hlen)));

  if(user) ssh_string_free_char(user);
  if(host) ssh_string_free_char(host);
  if(identity) ssh_string_free_char(identity);
  UNPROTECT(1);
  return out;
}

SEXP C_disconnect_session(SEXP ptr){
  ssh_disconnect(ssh_ptr_get(ptr));
  return R_NilValue;
}

SEXP C_libssh_version(){
  return Rf_mkString(SSH_STRINGIFY(LIBSSH_VERSION));
}
