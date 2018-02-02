#define R_NO_REMAP
#define STRICT_R_HEADERS

#include <Rinternals.h>
#include <libssh/callbacks.h>

void log_cb(int priority, const char *function, const char *buffer, void *userdata){
  REprintf("%s\n", buffer);
}

void R_init_ssh(DllInfo* info) {
  R_registerRoutines(info, NULL, NULL, NULL, NULL);
  R_useDynamicSymbols(info, TRUE);
  ssh_init();
  ssh_set_log_callback(log_cb);
  //ssh_set_log_level(SSH_OPTIONS_LOG_VERBOSITY);
}
