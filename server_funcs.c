#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>
#include <pthread.h>
#include <signal.h>
#include "blather.h"
#include <poll.h>

char serv_fifo [MAXPATH]; // server fifo name
mesg_t client_message;

client_t *server_get_client(server_t *server, int idx)
{
  return &(server->client[idx]); //return client at idx
}

void server_start(server_t *server, char *server_name, int perms)
{
  strcpy(server->server_name, server_name); //copy server name
  server->n_clients = 0; //initialize 0 client
  //printf("server start\n"); //debug
  strcpy(serv_fifo, server_name);
  strcat(serv_fifo, ".fifo"); //create server_name.fifo string
  remove(serv_fifo); //remove previous existing file
  mkfifo(serv_fifo, perms); //make fifo
  server->join_fd = open(serv_fifo, O_RDWR); //open and save fifo
  server->join_ready = 0; //initialize join_ready as 0
}

void server_shutdown(server_t *server)
{
  close(server->join_fd); //close fifo
  remove(serv_fifo); //remove fifo
  //printf("FIFO %s.fifo closed and removed\n")
  client_message.kind = BL_SHUTDOWN; //initialize shutdown message
  client_message.name[0] = '\0';
  client_message.body[0] = '\0';
  server_broadcast(server, &client_message); //broadcast shutdown
  for (int i = 0; i < (server->n_clients); )
  {
    //printf("shut down message sent\n");
    server_remove_client(server, i); //remove all client

  }
  //printf("server %s shut down\n", server_name);
}

int server_add_client(server_t *server, join_t *join)
{
  int pos = server->n_clients; //current index of nearest empty client
  if (server->n_clients < MAXCLIENTS)
  {
    strcpy(server_get_client(server, pos)->name, join->name); //set name for client
    strcpy(server_get_client(server, pos)->to_client_fname, join->to_client_fname); //copy fifo filename
    strcpy(server_get_client(server, pos)->to_server_fname, join->to_server_fname); //copy fifo filename
    server_get_client(server, pos)->to_client_fd
    = open(server_get_client(server, pos)->to_client_fname, O_RDWR); //open fifo
    server_get_client(server, pos)->to_server_fd
    = open(server_get_client(server, pos)->to_server_fname, O_RDWR); //open fifo
    server_get_client(server, pos)->data_ready = 0;
    server->n_clients += 1; //increase no of clients by 1
    return 0;
  }
  else {
    printf("There is no space for client\n");
    return (-1);
  }
}

int server_remove_client(server_t *server, int idx)
{
  close(server_get_client(server, idx)->to_client_fd); //close client fifos
  close(server_get_client(server, idx)->to_server_fd); //close client fifos
  //printf("client FIFOs closed\n")
  for (int i = idx; i < server->n_clients; i++)
  {
    server->client[i] = server->client[i+1]; //decrement client's index
  }
  //printf("Done shifting\n");
  server->n_clients -= 1; //decrease no of client by 1
  return 1;
}

int server_broadcast(server_t *server, mesg_t *mesg)
{
  for (int i = 0; i < server->n_clients; i++)
  {
    write(server_get_client(server, i)->to_client_fd, mesg, sizeof(mesg_t)); //write message to all to_client fifos
  }
  //printf("Broadcast successful\n");
  return 1;
}

void server_check_sources(server_t *server)
{
  struct pollfd data[server->n_clients]; //create poll strcut to check client
  for (int i = 0; i < server->n_clients; i++)
  {
    data[i].fd = server_get_client(server, i)->to_server_fd; //copy to_server fifo
    data[i].events = POLLIN;
  }
  // printf("create poll data\n");

  struct pollfd server_data[1]; //create poll struct to check for server join ready
  server_data[0].fd = server->join_fd;
  server_data[0].events = POLLIN;

  int ret_ser = poll(server_data, 1, 0); //poll server
  if(server->n_clients != 0){
    int ret = poll(data, server->n_clients, 0); //poll client
    if (ret < 0)
    {
      perror("poll() fail");
    }
    for (int i = 0; i < server->n_clients; i++)
    {
      if (data[i].revents & POLLIN) //client data ready
      {
        server->client[i].data_ready = 1; //set flag
        printf("Client Ready to read\n");
        //fflush(stdout);
      }
    }
  }
  if (ret_ser < 0){
    perror("join poll() fail");
  }

  if (server_data[0].revents & POLLIN) //server join ready
  {
    server->join_ready = 1; //set flag
    printf("Client Ready to join\n");
    //fflush(stdout);
  }
}


int server_join_ready(server_t *server)
{
  return server->join_ready; //return flag
  //printf("Join ready\n");
}

int server_handle_join(server_t *server)
{
  if (server_join_ready(server) == 1) //if flag is on
  {
    join_t join;
    read(server->join_fd, &join, sizeof(join_t)); //read 1 join request
    server_add_client(server, &join); //add client according to join request

    client_message.kind = BL_JOINED; // set message kind
    strcpy(client_message.name, join.name);
    client_message.body[0] = '\0';

    server_broadcast(server, &client_message); // broadcast new_client has joined

    //printf("Join successful\n");
    server->join_ready = 0;
  }
  return 1;
}

int server_client_ready(server_t *server, int idx)
{
  return server_get_client(server, idx)->data_ready; //return client flag
  //printf("client data ready\n");
}

int server_handle_client(server_t *server, int idx)
{
  if (server_client_ready(server, idx) == 1) //if client flag is on
  {
    mesg_t message;
    read(server_get_client(server, idx)->to_server_fd, &message, sizeof(mesg_t)); //read 1 message
    //printf("read data from client successful\n");
    if(message.kind == BL_MESG) //normal message
    {
      server_broadcast(server, &message); //broadcast the message
    }
    else if(message.kind == BL_DEPARTED){ //depart message
      server_remove_client(server, idx);  //remove the client
      server_broadcast(server, &message); //broadcast client has departed
    }
    server_get_client(server, idx)->data_ready = 0; // client's data_ready flage = 0
  }
  return 1;
}
