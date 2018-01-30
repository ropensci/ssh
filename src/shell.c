#include "myssh.h"

static void R_callback(SEXP fun, const char * buf, ssize_t len){
  if(!Rf_isFunction(fun)) return;
  int ok;
  SEXP str = PROTECT(Rf_allocVector(RAWSXP, len));
  memcpy(RAW(str), buf, len);
  SEXP call = PROTECT(Rf_lcons(fun, Rf_lcons(str, R_NilValue)));
  R_tryEval(call, R_GlobalEnv, &ok);
  UNPROTECT(2);
}

/* Set up tunnel to the target host */
SEXP C_ssh_exec(SEXP ptr, SEXP command, SEXP outfun, SEXP errfun){
  ssh_session ssh = ssh_ptr_get(ptr);
  ssh_channel channel = ssh_channel_new(ssh);
  bail_if(channel == NULL, "ssh_channel_new", ssh);
  bail_if(ssh_channel_open_session(channel), "ssh_channel_open_session", ssh);
  bail_if(ssh_channel_request_exec(channel, CHAR(STRING_ELT(command, 0))), "ssh_channel_request_exec", ssh);

  int nbytes;
  int status = NA_INTEGER;
  static char buffer[1024];
  while(ssh_channel_is_open(channel) && !ssh_channel_is_eof(channel)){
    if(pending_interrupt())
      goto cleanup;
    for(int stream = 0; stream < 2; stream++){
      while ((nbytes = ssh_channel_read_nonblocking(channel, buffer, sizeof(buffer), stream)) > 0)
        R_callback(stream ? errfun : outfun, buffer, nbytes);
      bail_if(nbytes == SSH_ERROR, "ssh_channel_read_nonblocking", ssh);
    }
  }
  //this blocks until command has completed
  status = ssh_channel_get_exit_status(channel);
cleanup:
  ssh_channel_close(channel);
  ssh_channel_free(channel);
  return Rf_ScalarInteger(status);
}
