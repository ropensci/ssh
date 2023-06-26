/* Convert modes to integer:
 * > as.integer(as.octmode(as.character('755')))
 * 493
 * > as.integer(as.octmode(as.character('644')))
 * 420
 */

#include <libgen.h>
#include <sys/stat.h>
#include "myssh.h"

static void assert_scp(int rc, const char * what, ssh_scp scp, ssh_session ssh){
  if (rc != SSH_OK){
    char buf[1024];
    strncpy(buf, ssh_get_error(ssh), 1023);
    ssh_scp_close(scp);
    ssh_scp_free(scp);
    Rf_errorcall(R_NilValue, "SCP failure: %s", buf);
  }
}

static void enter_directory(ssh_scp scp, char * path, ssh_session ssh){
  char subdir[4000];
  strncpy(subdir, basename(path), 3999);
  if(strcmp(path, basename(path)))
    enter_directory(scp, dirname(path), ssh);
  assert_scp(ssh_scp_push_directory(scp, subdir, 493L), "ssh_scp_push_directory", scp, ssh);
}

/* Example: https://github.com/jeroen/libssh/blob/master/examples/scp_download.c
 *
 */

static SEXP dirvec_to_r(char ** dirvec, int depth){
  SEXP out = PROTECT(Rf_allocVector(STRSXP, depth));
  for(int i = 0; i < depth; i++)
    SET_STRING_ELT(out, i, Rf_mkCharCE(dirvec[i], CE_UTF8));
  UNPROTECT(1);
  return out;
}

static SEXP stream_to_r(ssh_scp scp){
  R_xlen_t size = ssh_scp_request_get_size64(scp);
  SEXP out = Rf_allocVector(RAWSXP, size);
  unsigned char * ptr = RAW(out);
  size_t read_bytes = 0;
  do {
    if(pending_interrupt()){
      ssh_scp_deny_request(scp, "user interrupt");
      return NULL;
    }
    read_bytes = ssh_scp_read(scp, ptr, size);
    ptr += read_bytes;
    size -= read_bytes;
    //REprintf("\r Remaining: %lld bytes", size);
  } while(size != 0);
  return out;
}

/* Retrieve file from remote host */
SEXP C_scp_read_file(SEXP ptr, SEXP path){
  ssh_session ssh = ssh_ptr_get(ptr);
  ssh_scp scp = ssh_scp_new(ssh, SSH_SCP_READ, CHAR(STRING_ELT(path, 0)));
  assert_scp(ssh_scp_init(scp), "ssh_scp_init", scp, ssh);
  assert_scp(ssh_scp_pull_request(scp) != SSH_SCP_REQUEST_NEWFILE, "SSH_SCP_REQUEST_NEWFILE", scp, ssh);
  SEXP out = stream_to_r(scp);
  assert_scp(ssh_scp_pull_request(scp) != SSH_SCP_REQUEST_EOF, "SSH_SCP_REQUEST_EOF", scp, ssh);
  ssh_scp_close(scp);
  ssh_scp_free(scp);
  return out;
}

SEXP C_scp_write_file(SEXP ptr, SEXP path, SEXP data){
  ssh_session ssh = ssh_ptr_get(ptr);
  ssh_scp scp = ssh_scp_new(ssh, SSH_SCP_WRITE | SSH_SCP_RECURSIVE, ".");
  assert_scp(ssh_scp_init(scp), "ssh_scp_init", scp, ssh);
  char cpath[4000];
  char filename[4000];
  strncpy(cpath, CHAR(STRING_ELT(path, 0)), 3999);
  strncpy(filename, basename(cpath), 3999);
  if(strcmp(cpath, filename))
    enter_directory(scp, dirname(cpath), ssh);
  assert_scp(ssh_scp_push_file(scp, filename, Rf_xlength(data), 420L), "ssh_scp_push_file", scp, ssh);
  assert_scp(ssh_scp_write(scp, RAW(data), Rf_xlength(data)), "ssh_scp_write", scp, ssh);
  ssh_scp_close(scp);
  ssh_scp_free(scp);
  return path;
}

static void call_cb(SEXP data, SEXP cb, char * pwd[1000], int depth){
  //REprintf("Running call_cb\n");
  PROTECT(data);
  SEXP dir = PROTECT(dirvec_to_r(pwd, depth + 1));
  SEXP call = PROTECT(Rf_lcons(cb, Rf_lcons(data, Rf_lcons(dir, R_NilValue))));
  Rf_eval(call, R_GlobalEnv);
  UNPROTECT(3);
}

SEXP C_scp_download_recursive(SEXP ptr, SEXP path, SEXP cb){
  ssh_session ssh = ssh_ptr_get(ptr);
  ssh_scp scp = ssh_scp_new(ssh, SSH_SCP_READ | SSH_SCP_RECURSIVE, CHAR(STRING_ELT(path, 0)));
  assert_scp(ssh_scp_init(scp), "ssh_scp_init", scp, ssh);
  int status = SSH_OK;
  int depth = 0;
  char * pwd[1000];
  while(!pending_interrupt()){
    switch((status = ssh_scp_pull_request(scp))){
    case SSH_SCP_REQUEST_NEWFILE:
      assert_scp(ssh_scp_accept_request(scp), "ssh_scp_accept_request", scp, ssh);
      pwd[depth] = strdup(ssh_scp_request_get_filename(scp));
      SEXP data = stream_to_r(scp);
      if(data == NULL)
        goto cleanup;
      call_cb(data, cb, pwd, depth);
      free(pwd[depth]);
      break;
    case SSH_SCP_REQUEST_NEWDIR:
      ssh_scp_accept_request(scp);
      pwd[depth++] = strdup(ssh_scp_request_get_filename(scp));
      call_cb(R_NilValue, cb, pwd, depth - 1);
      break;
    case SSH_SCP_REQUEST_ENDDIR:
      free(pwd[--depth]);
      break;
    case SSH_SCP_REQUEST_WARNING:
      Rf_warningcall_immediate(R_NilValue, "SSH warning: %s\n",ssh_scp_request_get_warning(scp));
      break;
    case SSH_SCP_REQUEST_EOF:
      goto cleanup;
    case SSH_ERROR:
      assert_scp(status, "ssh_scp_pull_request", scp, ssh);
    default :
      Rf_error("Error in ssh_scp_pull_request case: %d\n", status);
    }
  }

cleanup:

  ssh_scp_close(scp);
  ssh_scp_free(scp);
  return R_NilValue;
}

SEXP C_scp_write_recursive(SEXP ptr, SEXP sources, SEXP sizes, SEXP paths, SEXP to, SEXP verbose){
  ssh_session ssh = ssh_ptr_get(ptr);
  ssh_scp scp = ssh_scp_new(ssh, SSH_SCP_WRITE | SSH_SCP_RECURSIVE, CHAR(STRING_ELT(to, 0)));
  assert_scp(ssh_scp_init(scp), "ssh_scp_init", scp, ssh);
  int depth = 0;
  char * pwd[1000];
  for(int i = 0; i < Rf_length(paths); i++){

    // check for SIGINT
    if(pending_interrupt())
      break;

    //calculate common path (find first non common subdir)
    SEXP filevec = VECTOR_ELT(paths, i);
    for(int j = 0; j < fmin(Rf_length(filevec), depth); j++){
      if(strcmp(pwd[j], CHAR(STRING_ELT(filevec, j)))){
        //cd up to here
        while(depth > j){
          ssh_scp_leave_directory(scp);
          free(pwd[--depth]);
        }
        break;
      }
    }

    //enter to subdir (not basename)
    for(int j = depth; j < Rf_length(filevec) - 1; j++){
      pwd[depth++] = strdup(CHAR(STRING_ELT(filevec, j)));
      assert_scp(ssh_scp_push_directory(scp, CHAR(STRING_ELT(filevec, j)), 493L), "ssh_scp_push_directory", scp, ssh);
    }

    //empty directories
    SEXP path = VECTOR_ELT(paths, i);
    SEXP file = STRING_ELT(path, Rf_length(path) - 1);
    if(file == NA_STRING)
      continue;

    // try to retain file mode
    struct stat perm = {0};
    if(stat(CHAR(STRING_ELT(sources, i)), &perm) < 0)
      Rf_error("Failed to get permissions for file %s", CHAR(STRING_ELT(sources, i)));
    int statchmod = perm.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);

    //create new file
    double size = REAL(sizes)[i];
    assert_scp(ssh_scp_push_file(scp, CHAR(file), size, statchmod), "ssh_scp_push_file", scp, ssh);

    //write file to channel
    int read = 0;
    long long total = 0;
    char buf[16384];
    FILE *fp = fopen(CHAR(STRING_ELT(sources, i)), "rb");
    if(!fp)
      Rf_error("Failed to open file %s", CHAR(STRING_ELT(sources, i)));
    do {
      assert_scp(ssh_scp_write(scp, buf, read), "ssh_scp_write", scp, ssh);
      read = fread(buf, sizeof(char), sizeof(buf), fp);
      total = total + read;
      if(size && Rf_asLogical(verbose))
        Rprintf("\r[%d%%] %s", (int) round(100 * total/size), CHAR(STRING_ELT(sources, i)));
    } while (read > 0);
    if(size && Rf_asLogical(verbose))
      Rprintf("\n");
    fclose(fp);
  }

  //cd back to root before exiting scp
  while(depth > 0){
    ssh_scp_leave_directory(scp);
    free(pwd[--depth]);
  }
  ssh_scp_close(scp);
  ssh_scp_free(scp);
  return to;
}
