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
#include <fcntl.h>
#include <string.h>

int* pipe_fds;

int read_from(int filedes) {
  char buffer[1024];
  int nbytes;

  nbytes = read (filedes, buffer, 1024);
  if (nbytes > 0){
    printf ("Server: got message: `%s'\n", buffer);
    write(filedes, buffer, nbytes);
  }
  return nbytes;
}

int setup_pipe(int* pdfs){
  if (pipe(pdfs) == -1){
    goto error;
  }

  int flags = 0;
  for (int i = 0; i < 2; i++){
    if ((flags = fcntl(pdfs[i], F_GETFL)) == -1){
      goto error;
    }
    flags |= O_NONBLOCK;
    if ((flags = fcntl(pdfs[i], F_SETFL)) == -1){
      goto error;
    }
  }
  return 0;

error:
  printf("Error setting up pipe: %s\n", strerror(errno));
  return -1;
}


void *worker(void* arg){
  THREADPOOL* tp = (THREADPOOL*) arg;
  fd_set active, read_only;
  FD_ZERO(&active);
  FD_SET(pipe_fds[0], &active);

  while (!threadpool_stopped(tp)){
    read_only = active;

    int ret= select (FD_SETSIZE, &read_only, NULL, NULL, NULL);
    switch (ret) {
       case -1:
        break;
      default:
        if (FD_ISSET(pipe_fds[0], &read_only)){
          // look into using just a pipe as queue.

          // read pipe to flush out the notifications. 
          char ch;
          read(pipe_fds[0], &ch, 10);
          // try to get a job from queue and process.
          int* job = threadpool_pop_no_wait(tp);
          if (job != NULL){
            if (*job >= FD_SETSIZE){
              printf("Error: got fd[%d] greater than FD_SETSIZE[%d]. Ignoring.\n", *job, FD_SETSIZE);
            }else{
              FD_SET(*job, &active);
            }
            free(job);
            job = NULL;
          }
        }
        for (int i = 0; i < FD_SETSIZE; i++){
          if (i == pipe_fds[0]){
            continue;
          }
          if (FD_ISSET(i, &read_only)){
            if (read_from(i) <= 0){
              printf("Disconnecting %d\n", i); 
              FD_CLR(i, &active);
              close(i);
            }
          }
        }
        break;
    } 
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
  pipe_fds = malloc(2*sizeof(int)); 
  setup_pipe(pipe_fds);

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

  printf("Serving on %d\n", port);
  while(1){
    read_fds = active_fds;

    int sel = select(FD_SETSIZE, &read_fds, NULL, NULL, NULL);
    switch (sel){
      case -1:
        break;
      default:
        for (int i = 0; i < FD_SETSIZE; i++){
          if (FD_ISSET(i, &read_fds)){
            if (i == serv){
              printf("New client connection. Put in queue.\n");
              struct sockaddr_in client;
              socklen_t size = sizeof(client);
              int client_fd = accept(serv, (struct sockaddr *) &client, &size);
              if (client_fd < 0){
                printf("Error: error accepting. %s\n", strerror(errno));
              }
              int *p = malloc(sizeof(*p));
              *p = client_fd;
              threadpool_push(tp,  (void*)p);
              // let threads know there is a new item for consumption
              if (write(pipe_fds[1], "r", 1) == -1 && errno != EAGAIN){
                printf("Error: couldn't write to pipe fd: %d.   %s\n", pipe_fds[1], strerror(errno));
              }
            }else{
              printf("Got input from fd[%d]. We only shoudl be listening for server fd[%d]\n", i, serv);
            }
          }
        }
        break;
    }
  }
}
