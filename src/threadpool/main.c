#include "threadpool.h"
#include "queue.h"
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/time.h>


int read_from(int filedes)
{
  char buffer[1024];
  int nbytes;

  nbytes = read (filedes, buffer, 1024);
  if (nbytes < 0){
    printf("Read error\n");
  } else if (nbytes == 0){
    return -1;
  }else{
    printf ("Server: got message: `%s'\n", buffer);
    write(filedes, buffer, nbytes);
    close(filedes);
  }
  return 0;
}



void *worker(void* arg){
  THREADPOOL* tp = (THREADPOOL*) arg;
  struct timeval timeout;
  fd_set active, read_only;
  FD_ZERO(&active);
  FD_ZERO(&read_only);
  //int count = 0;
  while (!threadpool_stopped(tp)){
    timeout.tv_sec = 0;
    timeout.tv_usec = 500;
    //printf("Setting select.  %d  thread[%d]\n", count++, getpid());
    read_only = active;

    int ret= select (FD_SETSIZE, &read_only, NULL, NULL, &timeout);
    switch (ret) {
      case 0:
        //timeout
//        printf("Timed out   %d.\n", count-1);
        break;
      case 1:
        // have input
        printf("Have input.\n");
        //TODO: READ

        for (int i = 0; i < FD_SETSIZE; i++){
          if (FD_ISSET(i, &read_only)){
             printf("Got input from fd[%d].\n", i);
             read_from(i);
             FD_CLR(i, &active);
          }
        }

        break;
      case -1:
        // error
       // printf("Error: error in select.\n");
        break;
    }
    // this pop should prolly have a timeout too )
    //printf("Waiting for pop.\n");
    int* job = threadpool_pop_no_wait(tp);
    if (job != NULL){
      if (*job >= FD_SETSIZE){
        printf("Error: got fd[%d] greater than FD_SETSIZE[%d]. Ignoring.\n", *job, FD_SETSIZE);
      }else{
        FD_SET(*job, &active);
        printf("Added %d to select in thread [%d]\n", *job, getpid());
      }
    }

    free(job);
    job = NULL;
  }
  return 0;
}


int tcp_server(int port)
{
  // Create socket
  int sock = socket (AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    goto error;
  }
  // set SO_REUSEADDR
  int reuse_addr = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr)) == -1){
    printf("Error: setsockopt SO_REUSEADDR error: %d\n", errno);
    goto error;
  }

  // Bind
  struct sockaddr_in addr;

  addr.sin_family = AF_INET;
  addr.sin_port = htons (port);
  addr.sin_addr.s_addr = htonl (INADDR_ANY);
  if (bind (sock, (struct sockaddr *) &addr, sizeof (addr)) < 0) {
    printf("Error: bind: %d\n",errno);
    goto error;
  }

  // Listen
  if (listen(sock, 127) == -1){
    printf("Error: listen error: %d\n", errno);
    goto error;
  }

  return sock;

error:
  if (sock >=0){
    close(sock);
  }
  return -1;
}

int main(){
  int port = 7878;
  int threads = 8;
  int queue_size = 10000;
  THREADPOOL* tp = threadpool_create(threads, queue_size);

  threadpool_set_func(tp, worker);

  threadpool_start(tp);

  int serv= tcp_server(port);

  fd_set read_fds, active_fds;
  FD_ZERO(&active_fds);
  FD_SET(serv, &active_fds);
  struct timeval timeout;

  while(1){
    read_fds = active_fds;
    timeout.tv_sec= 1;
    timeout.tv_usec = 500;

    printf("waiting for connect in main.\n");
    int sel = select(FD_SETSIZE, &read_fds, NULL, NULL, NULL);
    switch (sel){
      case -1:
//        printf("Error while selecting.\n");
        break;
      case 0:
//        printf("Timeout while selecting.\n");
        break;
      case 1:
//        printf("Have input.\n");
        for (int i = 0; i < FD_SETSIZE; i++){
          if (FD_ISSET(i, &read_fds)){
            if (i == serv){
              printf("New client connection. Put in queue.\n");
              struct sockaddr_in client;
              socklen_t size = sizeof(client);
              int client_fd = accept(serv, (struct sockaddr *) &client, &size);
              if (client_fd < 0){
                printf("Error: error accepting\n");
              }
              int *p = malloc(sizeof(*p));
              *p = client_fd;
              threadpool_push(tp,  (void*)p);
            }else{
              printf("Got input from fd[%d]. We only shoudl be listening for server fd[%d]\n", i, serv);
            }
          }
        }
        break;
      default:
        printf("Select returned [%d]. What to do?\n", sel);
    }
  }
}
