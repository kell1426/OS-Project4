/*kell1426
*04/26/18
*Daniel Kelly
*4718021*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdbool.h>
#include "makeargv.h"
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_NODES 100
#define MAX_CANDIDATES 100

void DAGCreator(node_t* n, char *filename);
void parseInputLine(char *buf, node_t* n, int line);
void returnWinner(void* args);
void countVotes(void* args);
void openPolls(void* args);
void addVotes(void* args);
void removeVotes(void* args);
void closePolls(void* args);




void parseInputLine(char *buf, node_t* n, int line)
{
  char **strings;
  int numOfTokens = makeargv(buf, ":", &strings);
  if(line == 1) //First read_line
  {
    int i;
    for(i = 0; i < numOfTokens; i++)
    {
      strcpy(n[i].name, strings[i]);
    }
    n[0].num_children = numOfTokens - 1;
    for(i = 1; i < numOfTokens; i++)
    {
      strcpy(n[0].childName[i - 1], n[i].name);
      strcpy(n[i].parentName, n[0].name);
    }
  }
  else
  {
    int i = 0;
    int parent = 0;
    while(n[i].name[0] != '\0')
    {
      char *name = &n[i].name;
      //char *name;
      //strcpy(name, n[i].name);
      if(strcmp(name, strings[0]) == 0)
      {
        parent = i;
      }
      i++;
    }
    n[parent].num_children = numOfTokens - 1;
    int j;
    for(j = 1; j < numOfTokens; j++)
    {
      strcpy(n[parent].childName[j - 1], strings[j]);
      strcpy(n[i + j - 1].name, strings[j]);
      strcpy(n[i + j - 1].parentName, n[parent].name);
    }
  }
}

void DAGCreator(node_t* n, char *filename)
{
  FILE *DAG = fopen(filename, "r");
  char *buf = malloc(255);
  int line = 1;
  while(fgets(buf, 255, DAG) != NULL)
  {
    if(strcmp(buf, "\n") != 0)
    {
      char* p = strchr(buf, '\n');//Delete trailing \n character.
  		if(p)
  		{
  			*p = 0;
  		}
      parseInputLine(buf, n, line);			//Call parseInputLine on this line.
      line++;
    }
  }
}

void returnWinner(void* args)
{
  struct threadArgs *realArgs = args;
  printf("Returning Winner\n");
  int clientSock =  realArgs->socket;
  send(clientSock, "Returning Winner\n", 256, 0);
  return;
}

void countVotes(void* args)
{
  struct threadArgs *realArgs = args;
  printf("Counting Votes\n");
  return;
}

void openPolls(void* args)
{
  struct threadArgs *realArgs = args;
  printf("Opening Polls\n");
  int clientSock =  realArgs->socket;
  send(clientSock, "Opening Polls\n", 256, 0);
  return;
}

void addVotes(void* args)
{
  struct threadArgs *realArgs = args;
  printf("Adding Votes\n");
  return;
}

void removeVotes(void* args)
{
  struct threadArgs *realArgs = args;
  printf("Removing Votes\n");
  return;
}

void closePolls(void* args)
{
  struct threadArgs *realArgs = args;
  printf("Closing Polls\n");
  return;
}



int main(int argc, char **argv){
  struct node* mainnodes=(struct node*)malloc(sizeof(struct node)*MAX_NODES);

	if (argc != 3){
		printf("Usage: ./server <DAG FILE> <Server Port>\n");
		return -1;
	}

  DAGCreator(mainnodes, argv[1]);
  int i = 0;
  while(mainnodes[i].name[0] != '\0')
  {
    mainnodes[i].pollOpen = false;
    i++;
  }
  //printgraph(mainnodes);

  int serverSock = socket(AF_INET, SOCK_STREAM, 0);

  struct sockaddr_in servAddress;
  servAddress.sin_family = AF_INET;
  servAddress.sin_port = htons(atoi(argv[2]));
  servAddress.sin_addr.s_addr = htonl(INADDR_ANY);
  bind(serverSock, (struct sockaddr *) &servAddress, sizeof(servAddress));

  listen(serverSock, 1);
  printf("Server listening on port %d\n", atoi(argv[2]));

  while(1)
  {
    struct sockaddr_in clientAddress;
    socklen_t size = sizeof(struct sockaddr_in);

    int clientSock = accept(serverSock, (struct sockaddr *) &clientAddress, &size);
    printf("Connection initiated from client at %s:%d\n", inet_ntoa(clientAddress.sin_addr), (int) ntohs(clientAddress.sin_port));


    int readNum = 0;
    while(1)
    {
      char *buffer = calloc(256, 1);
      int num = recv(clientSock, buffer, 256, 0);
      if(strcmp(buffer, "END") == 0)
      {
        break;
      }
      printf("Request received from client at %s:%d", inet_ntoa(clientAddress.sin_addr), (int) ntohs(clientAddress.sin_port));
      printf(", %s\n", buffer);
      struct threadArgs* args = malloc(sizeof(struct threadArgs));
      args->n = mainnodes;
      args->command = buffer;
      args->socket = clientSock;
      if(buffer[0] == 'R' && buffer[1] == 'W')
      {
        pthread_t thread;
        pthread_create(&thread, NULL, returnWinner, (void*) args);
      }
      else if(buffer[0] == 'C' && buffer[1] == 'V')
      {
        pthread_t thread;
        pthread_create(&thread, NULL, countVotes, (void*) args);
      }
      else if(buffer[0] == 'O' && buffer[1] == 'P')
      {
        pthread_t thread;
        pthread_create(&thread, NULL, openPolls, (void*) args);
      }
      else if(buffer[0] == 'A' && buffer[1] == 'V')
      {
        pthread_t thread;
        pthread_create(&thread, NULL, addVotes, (void*) args);
      }
      else if(buffer[0] == 'R' && buffer[1] == 'V')
      {
        pthread_t thread;
        pthread_create(&thread, NULL, removeVotes, (void*) args);
      }
      else if(buffer[0] == 'C' && buffer[1] == 'P')
      {
        pthread_t thread;
        pthread_create(&thread, NULL, closePolls, (void*) args);
      }
      //free(buffer);
    }


    close(clientSock);
    printf("Closed connection with client at %s:%d\n", inet_ntoa(clientAddress.sin_addr), (int) ntohs(clientAddress.sin_port));
  }

  close(serverSock);
  return 0;
}
