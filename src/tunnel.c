#include <Rinternals.h>
#include <libssh/libssh.h>
#include <string.h>
#include <stdlib.h>
#include <poll.h>

int open_port(int port);
#define make_string(x) x ? Rf_mkString(x) : ScalarString(NA_STRING)
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

void bail_if(int rc, const char * what, ssh_session ssh){
  if (rc != SSH_OK){
    const char * err = ssh_get_error(ssh);
    ssh_disconnect(ssh);
    ssh_free(ssh);
    Rf_errorcall(R_NilValue, "libssh failure at '%s': %s", what, err);
  }
}

size_t password_cb(SEXP rpass, const char * prompt, char buf[1024]){
  if(Rf_isString(rpass) && Rf_length(rpass)){
    strncpy(buf, CHAR(STRING_ELT(rpass, 0)), 1024);
    return Rf_length(STRING_ELT(rpass, 0));
  } else if(Rf_isFunction(rpass)){
    int err;
    SEXP call = PROTECT(LCONS(rpass, LCONS(make_string(prompt), R_NilValue)));
    SEXP res = PROTECT(R_tryEval(call, R_GlobalEnv, &err));
    if(err || !isString(res)){
      UNPROTECT(2);
      error("Password callback did not return a string value");
    }
    strncpy(buf, CHAR(STRING_ELT(res, 0)), 1024);
    UNPROTECT(2);
    return strlen(buf);
  }
  Rf_errorcall(R_NilValue, "unsupported password type");
}

int auth_password(ssh_session ssh, SEXP rpass){
  char buf[1024];
  password_cb(rpass, "Please enter your password", buf);
  int rc = ssh_userauth_password(ssh, NULL, buf);
  bail_if(rc == SSH_AUTH_ERROR, "password auth", ssh);
  return rc;
}

int auth_interactive(ssh_session ssh, SEXP rpass){
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
      char buf[1024];
      const char * prompt = ssh_userauth_kbdint_getprompt(ssh, iprompt, NULL);
      password_cb(rpass, prompt, buf);
      if (ssh_userauth_kbdint_setanswer(ssh, iprompt, buf) < 0)
        return SSH_AUTH_ERROR;
    }
    rc = ssh_userauth_kbdint(ssh, NULL, NULL);
  }
  return rc;
}

void tunnel_port(ssh_session ssh, int port, const char * outhost, int outport){
  ssh_channel tunnel = ssh_channel_new(ssh);
  bail_if(tunnel == NULL, "ssh_channel_new", ssh);
  bail_if(ssh_channel_open_forward(tunnel, outhost, outport, "localhost", port), "channel_open_forward", ssh);

  //open server
  int waitms = 200;
  int connfd = open_port(port);
  if(connfd > 0){
    struct pollfd ufds = {connfd, POLLIN, POLLIN};
    char buf[1024];
    while(1){
      R_CheckUserInterrupt();
      int avail = 0;
      while((avail = poll(&ufds, 1, waitms)) > 0){
        size_t outsize = read(connfd, buf, MIN(avail, sizeof(buf)));
        ssh_channel_write(tunnel, buf, outsize);
      }
      while((avail = ssh_channel_read_timeout(tunnel, buf, sizeof(buf), FALSE, waitms)) > 0){
        write(connfd, buf, avail);
      }
      if(ssh_channel_is_closed(tunnel) || ssh_channel_is_eof(tunnel)) break;
    }
    close(connfd);
  }
  ssh_channel_send_eof(tunnel);
  ssh_channel_close(tunnel);
  ssh_channel_free(tunnel);
}

SEXP C_blocking_tunnel(SEXP rhost, SEXP rport, SEXP ruser, SEXP rpass){
  int port = asInteger(rport);
  int verbosity = SSH_LOG_PROTOCOL;
  const char * host = CHAR(STRING_ELT(rhost, 0));
  const char * user = CHAR(STRING_ELT(ruser, 0));
  ssh_session ssh = ssh_new();
  bail_if(ssh_options_set(ssh, SSH_OPTIONS_HOST, host), "set host", ssh);
  bail_if(ssh_options_set(ssh, SSH_OPTIONS_USER, user), "set user", ssh);
  bail_if(ssh_options_set(ssh, SSH_OPTIONS_PORT, &port), "set port", ssh);
  bail_if(ssh_options_set(ssh, SSH_OPTIONS_LOG_VERBOSITY, &verbosity), "set verbosity", ssh);

  /* connect */
  bail_if(ssh_connect(ssh), "connect", ssh);

  /* TODO: authenticate server pubkey */
  //int state = ssh_is_server_known(ssh);
  ssh_key key;
  unsigned char * hash = NULL;
  size_t hlen = 0;
  ssh_get_publickey(ssh, &key);
  ssh_get_publickey_hash(key, SSH_PUBLICKEY_HASH_SHA1, &hash, &hlen);
  Rprintf("Server SHA1: %s\n", ssh_get_hexa(hash, hlen));

  /* authenticate client */
  if(ssh_userauth_none(ssh, NULL) != SSH_AUTH_SUCCESS){
    int method = ssh_userauth_list(ssh, NULL);
    if (method & SSH_AUTH_METHOD_PUBLICKEY && ssh_userauth_publickey_auto(ssh, NULL, NULL) == SSH_AUTH_SUCCESS){
      Rprintf("pubkey auth success!");
    } else if (method & SSH_AUTH_METHOD_INTERACTIVE && auth_interactive(ssh, rpass) == SSH_AUTH_SUCCESS) {
      Rprintf("interactive auth success!");
    } else if (method & SSH_AUTH_METHOD_PASSWORD && auth_password(ssh, rpass) == SSH_AUTH_SUCCESS) {
      Rprintf("password auth success!");
    }  else {
      Rf_error("Authentication failed, permission denied");
    }
  }

  /* display welcome message */
  char * banner = ssh_get_issue_banner(ssh);
  if(banner != NULL){
    Rprintf("%s\n", banner);
    free(banner);
  }

  /* TODO: do stuff */
  tunnel_port(ssh, 5555, "www.httpbin.org", 80);

  /* cleanups */
  ssh_disconnect(ssh);
  ssh_free(ssh);
  return R_NilValue;
}
