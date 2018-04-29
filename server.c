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
sem_t* sem;

void DAGCreator(node_t* n, char *filename);
void parseInputLine(char *buf, node_t* n, int line);
char* returnWinner(node_t* n, char* command);
char* countVotes(node_t* n, char* command);
void openPolls(node_t* n, node_t* node);
void addVotes(node_t* n, char* command);
void removeVotes(node_t* n, char* command);
void closePolls(node_t* n, node_t* node);

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

void serverFunction(void* args)
{
  struct serverArgs *realArgs = args;
  int clientSock = realArgs->socket;
  struct sockaddr_in clientAddress = realArgs->clientAddress;
  node_t* n = realArgs->n;
  printf("Connection initiated from client at %s:%d\n", inet_ntoa(clientAddress.sin_addr), (int) ntohs(clientAddress.sin_port));
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
    if(buffer[0] == 'R' && buffer[1] == 'W')
    {
      char* response = NULL;
      response = returnWinner(n, buffer);
      printf("Sending response to client at %s:%d", inet_ntoa(clientAddress.sin_addr), (int) ntohs(clientAddress.sin_port));
      printf(", %s\n", response);
      send(clientSock, response, 256, 0);
      free(response);
    }
    else if(buffer[0] == 'C' && buffer[1] == 'V')
    {
      char* response = NULL;
      response = countVotes(n, buffer);
      printf("Sending response to client at %s:%d", inet_ntoa(clientAddress.sin_addr), (int) ntohs(clientAddress.sin_port));
      printf(", %s\n", response);
      send(clientSock, response, 256, 0);
      free(response);
    }
    else if(buffer[0] == 'O' && buffer[1] == 'P')
    {
      sem_wait(&sem);
      char **strings;
      char *response = calloc(256, 1);
      int tokens = makeargv(buffer, ";", &strings);
      node_t* node = findnode(n, strings[1]);
      if(node->pollsOpen == true)
      {
        strcpy(response, "PF;\0");
      }
      else
      {
        //printf("Entering recursion\n");
        openPolls(n, node);
        //printf("Exited recursion\n");
        strcpy(response, "SC;\0");
      }
      sem_post(&sem);
      printf("Sending response to client at %s:%d", inet_ntoa(clientAddress.sin_addr), (int) ntohs(clientAddress.sin_port));
      printf(", %s\n", response);
      send(clientSock, response, 256, 0);
      free(response);
    }
    else if(buffer[0] == 'A' && buffer[1] == 'V')
    {
      addVotes(n, buffer);
    }
    else if(buffer[0] == 'R' && buffer[1] == 'V')
    {
      removeVotes(n, buffer);
    }
    else if(buffer[0] == 'C' && buffer[1] == 'P')
    {
      sem_wait(&sem);
      char **strings;
      char *response = calloc(256, 1);
      int tokens = makeargv(buffer, ";", &strings);
      node_t* node = findnode(n, strings[1]);
      if(node->pollsClosed == true)
      {
        strcpy(response, "PF;\0");
      }
      else
      {
        //printf("Entering recursion\n");
        closePolls(n, node);
        //printf("Exited recursion\n");
        strcpy(response, "SC;\0");
      }
      sem_post(&sem);
      printf("Sending response to client at %s:%d", inet_ntoa(clientAddress.sin_addr), (int) ntohs(clientAddress.sin_port));
      printf(", %s\n", response);
      send(clientSock, response, 256, 0);
      free(response);
    }
    //free(buffer);
  }
  close(clientSock);
  printf("Closed connection with client at %s:%d\n", inet_ntoa(clientAddress.sin_addr), (int) ntohs(clientAddress.sin_port));
}

char* returnWinner(node_t* n, char* command)
{
  //printf("Returning Winner\n");
  sem_wait(&sem);
  int i = 0;
  char* response = calloc(256, sizeof(char));
  while(n[i].name != '\0')
  {
    if(n[i].pollsClosed == false)
    {
      strcpy(response, "RO;");
      strcat(response, n[i].name);
      strcat(response, "\0");
      sem_post(&sem);
      return response;
    }
    i++;
  }
  int highestVotes = 0;
  char* winner = calloc(256, 1);
  i = 1;
  strcpy(winner, n[0].Candidates[0]);
  highestVotes = n[0].CandidatesVotes[0];
  while(n[0].Candidates[i][0] != 0)
  {
    if(n[0].CandidatesVotes[i] > highestVotes)
    {
      strcpy(winner, n[0].Candidates[i]);
      highestVotes = n[0].CandidatesVotes[i];
    }
    i++;
  }
  char buf[3];
  sprintf(buf, "%d", highestVotes);
  strcat(winner, ";");
  strcat(winner, buf);
  strcat(winner, "\0");
  strcpy(response, "SC;");
  strcat(response, winner);
  sem_post(&sem);
  return response;
}

char* countVotes(node_t* n, char* command)
{
  //printf("Counting Votes\n");
  sem_wait(&sem);
  char* response = calloc(256, sizeof(char));
  char **strings;
  int tokens = makeargv(command, ";", &strings);
  node_t* node = findnode(n, strings[1]);
  if(node->Candidates[0][0] == 0)
  {
    //Return no votes error code.
    strcpy(response, "No votes\0");
    sem_post(&sem);
    return response;
  }
  int highestVotes = 0;
  char* winner = calloc(256, 1);
  int i = 1;
  strcpy(winner, node->Candidates[0]);
  highestVotes = node->CandidatesVotes[0];
  while(node->Candidates[i][0] != 0)
  {
    if(node->CandidatesVotes[i] > highestVotes)
    {
      strcpy(winner, node->Candidates[i]);
      highestVotes = node->CandidatesVotes[i];
    }
    i++;
  }
  char buf[3];
  sprintf(buf, "%d", highestVotes);
  strcat(winner, ";");
  strcat(winner, buf);
  strcat(winner, "\0");
  strcpy(response, "SC;");
  strcat(response, winner);
  sem_post(&sem);
  return response;
}

void openPolls(node_t* n, node_t* node)
{
  node->pollsOpen = true;
  if(node->num_children > 0)
  {
    int i;
    for(i = 0; i < node->num_children; i++)
    {
      node_t* temp = findnode(n, node->childName[i]);
      openPolls(n, temp);
    }
  }
  return;
}

void addVotes(node_t* n, char* command)
{
  printf("Adding Votes\n");
  return;
}

void removeVotes(node_t* n, char* command)
{
  printf("Removing Votes\n");
  return;
}

void closePolls(node_t* n, node_t* node)
{
  node->pollsClosed = true;
  if(node->num_children > 0)
  {
    int i;
    for(i = 0; i < node->num_children; i++)
    {
      node_t* temp = findnode(n, node->childName[i]);
      closePolls(n, temp);
    }
  }
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
    mainnodes[i].pollsOpen = false;
    mainnodes[i].pollsClosed = false;
    int j;
    mainnodes[i].Candidates = calloc(MAX_CANDIDATES, sizeof(char*));
    for(j = 0; j < MAX_CANDIDATES; j++)
    {
      mainnodes[i].Candidates[j] = calloc(256, sizeof(char));
    }
    i++;
  }
  sem_init(&sem, 0, 1);
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
    pthread_t thread;
    struct serverArgs* args = malloc(sizeof(struct serverArgs));
    args->socket = clientSock;
    args->clientAddress = clientAddress;
    args->n = mainnodes;
    pthread_create(&thread, NULL, serverFunction, (void*) args);
  }

  close(serverSock);
  return 0;
}
