#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>
#include <pthread.h>
#include <signal.h>
#include "blather.h"

simpio_t simpio_actual;
simpio_t *simpio = &simpio_actual;

pthread_t user_thread;          // thread managing user input
pthread_t server_thread;

// client_t client;
join_t client_join;
mesg_t user_input_data;

int requests_fd, to_client_fd, to_server_fd;

void reset_mesg(mesg_t *mesg){
  mesg->name[0] = '\0';
  mesg->kind = 0;
  mesg->body[0] = '\0';
}
// Worker thread to manage user input
// The following function will read the input data from the user and send it to the server
// until the user press EOF. If the user press EOF, it will send DEPARTED message to the
// server so that every other user will see it.

void *user_worker(void *arg){
  while(!simpio->end_of_input){                                   // read data from user until user press EOF
    simpio_reset(simpio);
    iprintf(simpio, "");                                          // print prompt
    while(!simpio->line_ready && !simpio->end_of_input){          // read until line is complete
      simpio_get_char(simpio);
    }
    if(simpio->line_ready){
      reset_mesg(&user_input_data);                               // initialize the message
      user_input_data.kind = BL_MESG;                             // set kind of message
      strcpy(user_input_data.name, client_join.name);             // set name
      strcpy(user_input_data.body, simpio->buf);                  // set message

      write(to_server_fd, &user_input_data, sizeof(mesg_t));      // send typed data to server
    }
  }
  reset_mesg(&user_input_data);                                   // reinitialize the message data again
  user_input_data.kind = BL_DEPARTED;
  strcpy(user_input_data.name, client_join.name);
  write(to_server_fd, &user_input_data, sizeof(mesg_t));          // write depart message to the server
  pthread_cancel(server_thread);                                  // kill the server thread
  return NULL;
}


// Worker thread to listen to the info from the server.
// The following function will get data from the server if the server reply
// and print the messages to the console
// It will only stop working if the server is going to shutdown.
void *server_worker(void *arg){
 mesg_t reply;
 do{
  read(to_client_fd, &reply, sizeof(mesg_t));
  if(reply.kind == BL_DEPARTED){
    iprintf(simpio, "-- %s DEPARTED --\n",reply.name);
  }
  else if(reply.kind == BL_JOINED){
    iprintf(simpio, "-- %s JOINED --\n", reply.name);
  }
  else if(reply.kind == BL_MESG){
    iprintf(simpio, "[%s] : %s\n",reply.name,reply.body);
  }
  else if(reply.kind == BL_SHUTDOWN){
    iprintf(simpio, "!!! server is shutting down !!!\n");
  }
}while(reply.kind != BL_SHUTDOWN);                            // loop and read it until the server went to shutdown stage

 pthread_cancel(user_thread);                                 // kill the user_thread
 return NULL;
}

// The following function is the main function for the bl_client.c to run.
// It will get the data of server name and client name, then will create
// unique fifo files with own pid in its name. After it create unique fifo files
// it will send the data of its name, and fifo files to the server to communicate.
// It will communicate with the server using the functions that we provide above
// by creating its own user_thread to get data from user and server_thread to
// get data from the server.

int main(int argc, char *argv[]){
  if(argc < 3){
    printf("usage: %s <server> <name>\n",argv[0]);
    exit(1);
  }
  char server [MAXPATH];
  strcpy(server, argv[1]);                                  // main server fifo
  strcat(server, ".fifo");
  int ID = getpid();                                        // get unique ID
  char to_client_fname[MAXPATH], to_server_fname[MAXPATH];
  sprintf(to_client_fname, "%d", ID);
  strcat(to_client_fname, "-to-client.fifo");               // set fifo file with unique ID for server to message the client
  sprintf(to_server_fname, "%d", ID);
  strcat(to_server_fname, "-to-server.fifo");               // set fifo file with unique ID for client to message the server

  mkfifo(to_client_fname, DEFAULT_PERMS);                   // create fifo for server to write data
  mkfifo(to_server_fname, DEFAULT_PERMS);                   // create fifo for client to write data

  requests_fd = open(server, O_RDWR);                       // fd for join request to the server
  to_client_fd = open(to_client_fname, O_RDWR);             // fd to get data from server
  to_server_fd = open(to_server_fname, O_RDWR);             // fd for client to write data to server

  strcpy(client_join.name, argv[2]);                        // set client name
  strcpy(client_join.to_client_fname, to_client_fname);     // set client fifo file name
  strcpy(client_join.to_server_fname, to_server_fname);     // set server fifo file name

  write(requests_fd, &client_join, sizeof(join_t));         // writes client data to the server

  char prompt[MAXNAME];
  simpio_reset(simpio);                                     // initialize io
  snprintf(prompt, MAXNAME, "%s>> ",argv[2]);               // create a prompt string
  simpio_set_prompt(simpio, prompt);                        // set the prompt
  simpio_noncanonical_terminal_mode();                      // set the terminal into a compatible mode

  pthread_create(&user_thread,   NULL, user_worker,   NULL);     // start user thread to read input
  pthread_create(&server_thread, NULL, server_worker, NULL);     // start server thread to read data from server

  pthread_join(user_thread, NULL);
  pthread_join(server_thread, NULL);

  simpio_reset_terminal_mode();
  printf("\n");                                             // newline just to make returning to the terminal prettier

  close(requests_fd);
  close(to_client_fd);
  close(to_server_fd);
  return 0;
}
