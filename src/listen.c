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
void bail_for(int err, const char * what){
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
    bail_for(active < 0, "select()");
    if(active || pending_interrupt())
      break;
  }
  return active;
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
  bail_for(listenfd < 0, "socket()");
  bail_for(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0, "bind()");
  bail_for(listen(listenfd, 10) < 0, "listen()");

  //each accept() ye a new incoming connection
  Rprintf("Waiting for connetion on port %d...\n", port);
  wait_for_fd(listenfd);
  int connfd = accept(listenfd, NULL, NULL);
  bail_for(connfd < 0, "accept()");
  Rprintf("Incoming connection!\n");

  //do not allow additional client connetions
  close(listenfd);
  set_nonblocking(connfd);
  return connfd;
}
