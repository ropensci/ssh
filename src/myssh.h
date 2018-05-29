#define R_NO_REMAP
#define STRICT_R_HEADERS

#include <Rinternals.h>
#include <libssh/libssh.h>
#include <libssh/callbacks.h>
#include <stdlib.h>
#include <string.h>

/* function has been renamed */
#if defined(LIBSSH_VERSION_INT) && LIBSSH_VERSION_INT >= SSH_VERSION_INT(0,8,0)
#define myssh_get_publickey ssh_get_server_publickey
#else
#define myssh_get_publickey ssh_get_publickey
#endif

#define make_string(x) x ? Rf_mkString(x) : Rf_ScalarString(NA_STRING)
ssh_session ssh_ptr_get(SEXP ptr);
int pending_interrupt();
void assert_channel(int rc, const char * what, ssh_channel channel);
