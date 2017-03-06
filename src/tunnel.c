#define R_NO_REMAP
#define STRICT_R_HEADERS

#include <Rinternals.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif

#include <libssh/libssh.h>

#define make_string(x) x ? Rf_mkString(x) : Rf_ScalarString(NA_STRING)

#ifdef _WIN32
#define NONBLOCK_OK (WSAGetLastError() == WSAEWOULDBLOCK)
void set_nonblocking(int sockfd){
  u_long nonblocking = 1;
  ioctlsocket(sockfd, FIONBIO, &nonblocking);
}
#else
#define NONBLOCK_OK (errno == EAGAIN || errno == EWOULDBLOCK)
void set_nonblocking(int sockfd){
  long arg = fcntl(sockfd, F_GETFL, NULL);
  arg |= O_NONBLOCK;
  fcntl(sockfd, F_SETFL, arg);
}
#endif

/* Check for interrupt without long jumping */
void check_interrupt_fn(void *dummy) {
  R_CheckUserInterrupt();
}

int pending_interrupt() {
  return !(R_ToplevelExec(check_interrupt_fn, NULL));
}

/* check for system errors */
void syserror_if(int err, const char * what){
  if(err && !NONBLOCK_OK)
    Rf_errorcall(R_NilValue, "System failure for: %s (%s)", what, strerror(errno));
}

/* Wait for descriptor */
int wait_for_fd(int fd){
  int waitms = 200;
  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = waitms * 1000;
  fd_set rfds;
  int active = 0;
  while(active == 0){
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    active = select(fd+1, &rfds, NULL, NULL, &tv);
    syserror_if(active < 0, "select()");
    if(active || pending_interrupt())
      break;
  }
  return active;
}

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
    SEXP call = PROTECT(Rf_lcons(rpass, Rf_lcons(make_string(prompt), R_NilValue)));
    SEXP res = PROTECT(R_tryEval(call, R_GlobalEnv, &err));
    if(err || !Rf_isString(res)){
      UNPROTECT(2);
      Rf_error("Password callback did not return a string value");
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

int open_port(int port){

  // define server socket
  struct sockaddr_in serv_addr;
  memset(&serv_addr, '0', sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(port);

  //creates the listening socket
  int listenfd = socket(AF_INET, SOCK_STREAM, 0);
  syserror_if(listenfd < 0, "socket()");
  syserror_if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0, "bind()");
  syserror_if(listen(listenfd, 10) < 0, "listen()");
  return listenfd;
}






void host_tunnel(ssh_channel tunnel, int connfd){
  int waitms = 200;
  char buf[1024];

  //assume connfd is non-blocking
  while(!pending_interrupt() && ssh_channel_is_open(tunnel) && !ssh_channel_is_eof(tunnel)){
    Rprintf(".");
    int avail = 1;
    while((avail = recv(connfd, buf, sizeof(buf), 0)) > 0)
      ssh_channel_write(tunnel, buf, avail);
    syserror_if(avail == -1, "recv() from user");
    while((avail = ssh_channel_read_timeout(tunnel, buf, sizeof(buf), FALSE, waitms)) > 0)
      syserror_if(send(connfd, buf, avail, 0) < avail, "send() to user");
    syserror_if(avail == -1, "ssh_channel_read_timeout()");
  }
  Rprintf("closing connfd\n");
  close(connfd);
  Rprintf("closing channel...");
  ssh_channel_send_eof(tunnel);
  ssh_channel_close(tunnel);
  ssh_channel_free(tunnel);
  Rprintf("done!\n");

  //TODO: suicide fork
}


void open_tunnel(ssh_session ssh, int port, const char * outhost, int outport){
  Rprintf("Waiting for connetion on port %d...\n", port);

  int listenfd = open_port(port);
  //while(!pending_interrupt()){
    wait_for_fd(listenfd);
    int connfd = accept(listenfd, NULL, NULL);
    syserror_if(connfd < 0, "accept()");

    Rprintf("Incoming connection!\n");
    set_nonblocking(connfd);
    ssh_channel tunnel = ssh_channel_new(ssh);
    bail_if(tunnel == NULL, "ssh_channel_new", ssh);
    bail_if(ssh_channel_open_forward(tunnel, outhost, outport, "localhost", port), "channel_open_forward", ssh);

    //TODO fork here
    host_tunnel(tunnel, connfd);
  //}
  Rprintf("closing listenfd...");
  close(listenfd);
  Rprintf("done!\n");
}

SEXP C_blocking_tunnel(SEXP rhost, SEXP rport, SEXP ruser, SEXP thost, SEXP tport, SEXP lport, SEXP rpass){
  int port = Rf_asInteger(rport);
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

  /* get server identity */
  ssh_key key;
  unsigned char * hash = NULL;
  size_t hlen = 0;
  ssh_get_publickey(ssh, &key);
  ssh_get_publickey_hash(key, SSH_PUBLICKEY_HASH_SHA1, &hash, &hlen);
  int state = ssh_is_server_known(ssh);
  Rprintf("%s server SHA1: %s\n", state ? "known" : "unknown", ssh_get_hexa(hash, hlen));

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

  /* Set up tunnel to the target host */
  open_tunnel(ssh, Rf_asInteger(lport), CHAR(STRING_ELT(thost, 0)), Rf_asInteger(tport));

  /* cleanups */
  Rprintf("clossing ssh session...\n");
  ssh_disconnect(ssh);
  ssh_free(ssh);
  return R_NilValue;
}
