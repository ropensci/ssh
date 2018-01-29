#include "myssh.h"

/* Set up tunnel to the target host */
SEXP C_ssh_exec(SEXP ptr, SEXP command){
  ssh_session ssh = ssh_ptr_get(ptr);
  ssh_channel channel = ssh_channel_new(ssh);
  bail_if(channel == NULL, "ssh_channel_new", ssh);
  bail_if(ssh_channel_open_session(channel), "ssh_channel_open_session", ssh);
  bail_if(ssh_channel_request_exec(channel, CHAR(STRING_ELT(command, 0))), "ssh_channel_request_exec", ssh);

  char buffer[256];
  int nbytes;
  nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0);
  while (nbytes > 0){
    Rprintf("%.*s", nbytes, buffer);
    nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0);
  }
  nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 1);
  while (nbytes > 0){
    REprintf("%.*s", nbytes, buffer);
    nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 1);
  }
  int status = ssh_channel_get_exit_status(channel);
  ssh_channel_close(channel);
  ssh_channel_free(channel);
  return Rf_ScalarInteger(status);
}
