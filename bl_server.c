#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>
#include <pthread.h>
#include <signal.h>
#include "blather.h"
#include <poll.h>

server_t server;

void handle_SIGINT(int sig_num) {
  //printf("\nShutting down.\n");
  server_shutdown(&server); //shut down server
  exit(0);
}

void handle_SIGTERM(int sig_num) {
  // printf("\nShutting down.\n");
  server_shutdown(&server); //shutdown server
  exit(0);
}

int main(int argc, char *argv[])
{
  struct sigaction my_sa = {}; //init signal handler
  sigemptyset(&my_sa.sa_mask);

  my_sa.sa_handler = handle_SIGTERM;
  sigaction(SIGTERM, &my_sa, NULL); //handle SIGTERM

  my_sa.sa_handler = handle_SIGINT;
  sigaction(SIGINT,  &my_sa, NULL); //handle SIGINT

  if(argc < 2) //wrong usage
  {
    printf("usage: %s <server_name>\n",argv[0]); //correct usage
    exit(1);
  }

  server_start(&server, argv[1], DEFAULT_PERMS); //initialize new server

  while (1)
  {
    server_check_sources(&server); //check if ready to join/data ready
    //fflush(stdout);
    server_handle_join(&server); //handle join request
    for (int i = 0; i < (server.n_clients); i++)
    {
      server_handle_client(&server, i); //handle client data
    }
  }
}
