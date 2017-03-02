#include <Rinternals.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>

/* Check for interrupt without long jumping */
void check_interrupt_fn(void *dummy) {
  R_CheckUserInterrupt();
}

int pending_interrupt() {
  return !(R_ToplevelExec(check_interrupt_fn, NULL));
}

/* check for system errors */
void bail_for(int err, const char * what){
  if(err)
    Rf_errorcall(R_NilValue, "System failure for: %s (%s)", what, strerror(errno));
}

int open_port(int port){

  // define server socket
  struct sockaddr_in serv_addr;
  memset(&serv_addr, '0', sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(port);

  //creates the listening socket
  int reuse = 1;
  int listenfd = socket(AF_INET, SOCK_STREAM, 0);
  setsockopt(listenfd, SOL_SOCKET,SO_REUSEADDR, &reuse, sizeof(reuse));
  bail_for(listenfd < 0, "socket()");
  bail_for(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0, "bind()");
  bail_for(listen(listenfd, 10) < 0, "listen()");

  //each accept() ye a new incoming connection
  Rprintf("Waiting for connetion on port %d...\n", port);

  //TODO: should poll() instead of block
  int waitms = 500;
  struct pollfd ufds = {listenfd, POLLIN, POLLIN};
  while(!poll(&ufds, 1, waitms)){
    if(pending_interrupt()){
      close(listenfd);
      return -1;
    }
  }
  int connfd = accept(listenfd, NULL, NULL);
  bail_for(connfd < 0, "accept()");
  Rprintf("Incoming connection!\n");

  //do not allow additional client connetions
  close(listenfd);
  return connfd;
}
