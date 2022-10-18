#define R_NO_REMAP
#define STRICT_R_HEADERS

#include <Rinternals.h>
#include <R_ext/Rdynload.h>
#include <R_ext/Visibility.h>
#include <libssh/callbacks.h>

/* .Call calls */
extern SEXP C_blocking_tunnel(SEXP, SEXP, SEXP, SEXP);
extern SEXP C_disconnect_session(SEXP);
extern SEXP C_libssh_version(void);
extern SEXP C_scp_download_recursive(SEXP, SEXP, SEXP);
extern SEXP C_scp_read_file(SEXP, SEXP);
extern SEXP C_scp_write_file(SEXP, SEXP, SEXP);
extern SEXP C_scp_write_recursive(SEXP, SEXP, SEXP, SEXP, SEXP, SEXP);
extern SEXP C_ssh_exec(SEXP, SEXP, SEXP, SEXP);
extern SEXP C_ssh_info(SEXP);
extern SEXP C_start_session(SEXP, SEXP, SEXP, SEXP, SEXP, SEXP);
extern SEXP R_ssh_new_file_writer(SEXP);
extern SEXP R_ssh_total_writers(void);
extern SEXP R_ssh_write_file_writer(SEXP, SEXP, SEXP);

static const R_CallMethodDef CallEntries[] = {
  {"C_blocking_tunnel",        (DL_FUNC) &C_blocking_tunnel,        4},
  {"C_disconnect_session",     (DL_FUNC) &C_disconnect_session,     1},
  {"C_libssh_version",         (DL_FUNC) &C_libssh_version,         0},
  {"C_scp_download_recursive", (DL_FUNC) &C_scp_download_recursive, 3},
  {"C_scp_read_file",          (DL_FUNC) &C_scp_read_file,          2},
  {"C_scp_write_file",         (DL_FUNC) &C_scp_write_file,         3},
  {"C_scp_write_recursive",    (DL_FUNC) &C_scp_write_recursive,    6},
  {"C_ssh_exec",               (DL_FUNC) &C_ssh_exec,               4},
  {"C_ssh_info",               (DL_FUNC) &C_ssh_info,               1},
  {"C_start_session",          (DL_FUNC) &C_start_session,          6},
  {"R_ssh_new_file_writer",    (DL_FUNC) &R_ssh_new_file_writer,    1},
  {"R_ssh_total_writers",      (DL_FUNC) &R_ssh_total_writers,      0},
  {"R_ssh_write_file_writer",  (DL_FUNC) &R_ssh_write_file_writer,  3},
  {NULL, NULL, 0}
};

static void log_cb(int priority, const char *function, const char *buffer, void *userdata){
  REprintf("%s\n", buffer);
}

attribute_visible void R_init_ssh(DllInfo* dll) {
  R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
  R_useDynamicSymbols(dll, FALSE);
  ssh_init();
  ssh_set_log_callback(log_cb);
  //ssh_set_log_level(SSH_OPTIONS_LOG_VERBOSITY);
}
