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
#include <pthread.h>

pthread_mutex_t fd_mutex;

typedef struct readjob {
  fd_set* actives;
  int current_fd;
} readjob;

void setclr_bit(int b, fd_set* s, bool set){
  if (b > FD_SETSIZE){
    printf("Error: fd[%d] is greater than FD_SETSIZE[%d]\n", b, FD_SETSIZE);
  }
  pthread_mutex_lock(&fd_mutex);
  if (set){
    FD_SET(b, s);
  }else{
    FD_CLR(b, s);
  }
  pthread_mutex_unlock(&fd_mutex);
}

int read_from(readjob* j)
{
  char buffer[1024];
  int nbytes;

  nbytes = read (j->current_fd, buffer, 1024);
  if (nbytes == 0){
    return 0;
  }if (nbytes < 0){
    printf("Read error with [%d]\n", j->current_fd);
    // FD_* actions probably need mutex.
    setclr_bit(j->current_fd, j->actives, false);
    close(j->current_fd);
    return -1;
  }
//  printf ("Server: got message: `%s'\n", buffer);
  write(j->current_fd, buffer, nbytes);
  return nbytes;
}


void *worker(void* arg){
  THREADPOOL* tp = (THREADPOOL*) arg;
  while (!threadpool_stopped(tp)){
    readjob* job = threadpool_pop(tp);
    if (job != NULL){
      printf("fd[%d] is ready for reading.\n", job->current_fd);
      read_from(job) ;

    }

    free(job);
    job = NULL;
  }
  return 0;
}


int set_nonblock(int fd) {
  int flags;
  if ((flags = fcntl(fd, F_GETFL, 0)) == -1){
    flags = 0;
  }
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int tcp_server(int port)
{
  // Create socket
  int sock = socket (AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    goto error;
  }
  set_nonblock(sock);
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
  if (listen(sock, 128) == -1){
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

void eventloop(int serv, fd_set* active_fds, THREADPOOL* tp){
  fd_set read_fds;
  while(1){
    pthread_mutex_lock(&fd_mutex);
    read_fds = *active_fds;
    pthread_mutex_unlock(&fd_mutex);

    printf("waiting for connect in main.\n");
 
    int retval = select(FD_SETSIZE, &read_fds, NULL, NULL, NULL) ;
    if (retval > 0){
        for (int i = 0; i < FD_SETSIZE; i++){
          if (FD_ISSET(i, &read_fds)){
            if (i == serv){
              printf("New client connection. Put in queue.\n");
              struct sockaddr_in client;
              socklen_t size = sizeof(client);
              int client_fd = accept(serv, (struct sockaddr *) &client, &size);
              if (client_fd < 0){
                printf("Error: error accepting\n");
              }else{
                setclr_bit(client_fd, active_fds, true);
              }
            }else{
              printf("Got input from fd[%d]. Push off to threadpool.\n", i);
              readjob* job = malloc(sizeof(*job));
              job->actives = active_fds;
              job->current_fd = i;
              threadpool_push(tp, (void*) job);
            }
          }
        }
    }else{
      printf("Select retval: %d.   errno: %d\n", retval, errno);
      perror(errno);
    }
  }
}

int main(){
  int port = 7878;
  int threads = 10;
  int queue_size = 5000;
  if ((pthread_mutex_init(&fd_mutex, NULL)) != 0){
    printf("Error: couldnt initialize fd_mutex.\n");
    exit(1);
  }

  THREADPOOL* tp = threadpool_create(threads, queue_size);

  threadpool_set_func(tp, worker);

  threadpool_start(tp);

  int serv= tcp_server(port);

  fd_set* active_fds = malloc(sizeof(fd_set));
  FD_ZERO(active_fds);
  setclr_bit(serv, active_fds, true); 
  eventloop(serv, active_fds, tp);
}
