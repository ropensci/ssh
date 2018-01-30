#include "myssh.h"

/* Retrieve file from remote host */
SEXP C_scp_read_file(SEXP ptr, SEXP path){
  ssh_session ssh = ssh_ptr_get(ptr);
  ssh_scp scp = ssh_scp_new(ssh, SSH_SCP_READ, CHAR(STRING_ELT(path, 0)));
  bail_if(ssh_scp_init(scp) != SSH_OK, "ssh_scp_init", ssh);
  bail_if(ssh_scp_pull_request(scp) != SSH_SCP_REQUEST_NEWFILE, "ssh_scp_pull_request", ssh);

  /* get file info */
  R_xlen_t size = ssh_scp_request_get_size64(scp);
  ssh_scp_accept_request(scp);
  SEXP out = Rf_allocVector(RAWSXP, size);
  ssh_scp_accept_request(scp);
  if(ssh_scp_read(scp, RAW(out), size) != size)
    Rf_error("Read bytes did not match filesize");
  ssh_scp_close(scp);
  ssh_scp_free(scp);
  return out;
}
