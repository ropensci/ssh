#include "myssh.h"
#include <sys/stat.h>
#include <libgen.h>


/* Retrieve file from remote host */
SEXP C_scp_read_file(SEXP ptr, SEXP path){
  ssh_session ssh = ssh_ptr_get(ptr);
  ssh_scp scp = ssh_scp_new(ssh, SSH_SCP_READ, CHAR(STRING_ELT(path, 0)));
  bail_if(ssh_scp_init(scp), "ssh_scp_init", ssh);
  bail_if(ssh_scp_pull_request(scp) != SSH_SCP_REQUEST_NEWFILE, "ssh_scp_pull_request", ssh);

  /* get file info */
  R_xlen_t size = ssh_scp_request_get_size64(scp);
  SEXP out = Rf_allocVector(RAWSXP, size);
  bail_if(ssh_scp_accept_request(scp), "ssh_scp_accept_request", ssh);
  if(ssh_scp_read(scp, RAW(out), size) != size)
    Rf_error("Read bytes did not match filesize");
  ssh_scp_close(scp);
  ssh_scp_free(scp);
  return out;
}

/* Convert modes to integer:
 * > as.integer(as.octmode(as.character('755')))
 * 493
 * > as.integer(as.octmode(as.character('644')))
 * 420
 */


void enter_directory(ssh_scp scp, char * path, ssh_session ssh){
  char subdir[4000];
  strncpy(subdir, basename(path), 4000);
  if(strcmp(path, basename(path)))
    enter_directory(scp, dirname(path), ssh);
  bail_if(ssh_scp_push_directory(scp, subdir, 493L), "ssh_scp_push_directory", ssh);
}

SEXP C_scp_write_file(SEXP ptr, SEXP path, SEXP data){
  ssh_session ssh = ssh_ptr_get(ptr);
  ssh_scp scp = ssh_scp_new(ssh, SSH_SCP_WRITE | SSH_SCP_RECURSIVE, ".");
  bail_if(ssh_scp_init(scp), "ssh_scp_init", ssh);
  char * cpath = (char*) CHAR(STRING_ELT(path, 0));
  if(strcmp(cpath, basename(cpath)))
    enter_directory(scp, dirname(cpath), ssh);
  bail_if(ssh_scp_push_file(scp, basename(cpath), Rf_length(data), 420L), "ssh_scp_push_file", ssh);
  bail_if(ssh_scp_write(scp, RAW(data), Rf_length(data)), "ssh_scp_write", ssh);
  ssh_scp_close(scp);
  ssh_scp_free(scp);
  return path;
}
