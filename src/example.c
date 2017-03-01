#include <Rinternals.h>
#include <libssh/libssh.h>
#include <string.h>
#include <stdlib.h>

void bail_if(int rc, const char * what, ssh_session ssh){
  if (rc != SSH_OK){
    const char * err = ssh_get_error(ssh);
    ssh_disconnect(ssh);
    ssh_free(ssh);
    Rf_errorcall(R_NilValue, "libssh failure at '%s': %s", what, err);
  }
}

int test_forwarding(ssh_session ssh);

SEXP C_blocking_tunnel(SEXP rhost, SEXP rport, SEXP ruser){
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
    if (method & SSH_AUTH_METHOD_PUBLICKEY){
      bail_if(ssh_userauth_publickey_auto(ssh, NULL, NULL) != SSH_AUTH_SUCCESS, "public key authentication", ssh);
    } else {
      Rf_error("No suitable authentication method implemented");
    }
  }


  /* display welcome message */
  char * banner = ssh_get_issue_banner(ssh);
  if(banner != NULL){
    Rprintf("%s\n", banner);
    free(banner);
  }

  /* TODO: do stuff */
  test_forwarding(ssh);

  /* cleanups */
  ssh_disconnect(ssh);
  ssh_free(ssh);
  return R_NilValue;
}

int test_forwarding(ssh_session ssh){
  char * http_get = "GET /get HTTP/1.1\nHost: www.httpbin.org\n\n";
  int nbytes, nwritten;
  ssh_channel tunnel = ssh_channel_new(ssh);
  bail_if(tunnel == NULL, "ssh_channel_new", ssh);
  bail_if(ssh_channel_open_forward(tunnel, "www.httpbin.org", 80, "localhost", 5555),
          "channel_open_forward", ssh);
  nbytes = strlen(http_get);
  nwritten = ssh_channel_write(tunnel, http_get, nbytes);
  if (nbytes != nwritten) {
    Rf_error("Failure on ssh_channel_write");
  }
  char dest[1000];
  int len = 0;
  while(1){
    len = ssh_channel_read_timeout(tunnel, dest, 1000, FALSE, 1000);
    if(len <= 0 || ssh_channel_is_eof(tunnel) || ssh_channel_is_closed(tunnel))
      break;
    Rprintf("%.*s", len, dest);
  }
  ssh_channel_send_eof(tunnel);
  ssh_channel_close(tunnel);
  ssh_channel_free(tunnel);
  return SSH_OK;
}
