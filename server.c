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
void addVotes(node_t* n, char* command, node_t* root);
char* removeVotes(node_t* n, char* command, node_t* root);
void closePolls(node_t* n, node_t* node);

//Function parseInputLine
//Takes in one line of the file, the nodes, and the line number.
//Adds in necessary info to the nodes and creates
//the parent/child links
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

//Function DAGCreator
//Takes in the nodes and the DAG file .
//Passes each line of the DAG file to the function
//parseInputLine.
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

//Function serverFunction
//Takes in the thread arguments
//This function is called for each thread generated
//For each client that connects to the server, a thread is
//spawned and calls this function.
//Receives the command from the client and parses
//the command to determine which of the six requests it is
//Based on the command, it calls the corresponding function
//and then sends the response back to the client.
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
    printf("Request received from client at %s:%d, ", inet_ntoa(clientAddress.sin_addr), (int) ntohs(clientAddress.sin_port));
    char** strings3;
    int tok = makeargv(buffer, ";", &strings3);
    char *trimmed = calloc(20, 1);
    if(strings3[1] != NULL)
    {
      trimmed = trimwhitespace(strings3[1]);
    }
    if(strcmp(strings3[0], "RW") == 0)
    {
      printf("RW %s %s\n", NULL, NULL);
    }
    else
    {
      printf("%s %s %s\n", strings3[0], trimmed, strings3[2]);
    }
    if(buffer[0] == 'R' && buffer[1] == 'W')
    {
      sem_wait(&sem);
      char* response = NULL;
      response = returnWinner(n, buffer);
      printf("Sending response to client at %s:%d, ", inet_ntoa(clientAddress.sin_addr), (int) ntohs(clientAddress.sin_port));
      int b = 0;
      while(response[b] != '\0')
      {
        if(response[b] == ';')
        {
          if(response[b+1] == '\0')
          {
            printf(" %s", NULL);
            break;
          }
          else
          {
            printf(" ");
          }
        }
        else
        {
          printf("%c", response[b]);
        }
        b++;
      }
      printf("\n");
      send(clientSock, response, 256, 0);
      free(response);
      sem_post(&sem);
    }
    else if(buffer[0] == 'C' && buffer[1] == 'V')
    {
      sem_wait(&sem);
      char* response = NULL;
      response = countVotes(n, buffer);
      printf("Sending response to client at %s:%d, ", inet_ntoa(clientAddress.sin_addr), (int) ntohs(clientAddress.sin_port));
      int b = 0;
      while(response[b] != '\0')
      {
        if(response[b] == ';')
        {
          if(response[b+1] == '\0')
          {
            printf(" %s", NULL);
            break;
          }
          else
          {
            printf(" ");
          }
        }
        else
        {
          printf("%c", response[b]);
        }
        b++;
      }
      printf("\n");
      send(clientSock, response, 256, 0);
      free(response);
      sem_post(&sem);
    }
    else if(buffer[0] == 'O' && buffer[1] == 'P')
    {
      sem_wait(&sem);
      char **strings;
      char *response = calloc(256, 1);
      int tokens = makeargv(buffer, ";", &strings);
      char* trimmed = calloc(20, 1);
      trimmed = trimwhitespace(strings[1]);
      int a = 0;
      int validRegion = 0;
      while(n[a].name[0] != 0)
      {
        if(strcmp(n[a].name, trimmed) == 0)
        {
          validRegion = 1;
          break;
        }
        a++;
      }
      if(validRegion == 0)
      {
        strcpy(response, "NR;");
        strcat(response, trimmed);
        strcat(response, "\0");
      }
      else
      {
        node_t* node = findnode(n, trimmed);
        if(node->pollsOpen == true && node->pollsClosed == false)
        {
          strcpy(response, "PF;");
          strcat(response, trimmed);
          strcat(response, " Open\0");
        }
        else if((node->pollsOpen == true && node->pollsClosed == true) || node->Candidates[0][0] != 0)
        {
          strcpy(response, "RR;");
          strcat(response, trimmed);
          strcat(response, "\0");
        }
        else
        {
          //printf("Entering recursion\n");
          openPolls(n, node);
          //printf("Exited recursion\n");
          strcpy(response, "SC;\0");
        }
      }
      printf("Sending response to client at %s:%d, ", inet_ntoa(clientAddress.sin_addr), (int) ntohs(clientAddress.sin_port));
      int b = 0;
      while(response[b] != '\0')
      {
        if(response[b] == ';')
        {
          if(response[b+1] == '\0')
          {
            printf(" %s", NULL);
            break;
          }
          else
          {
            printf(" ");
          }
        }
        else
        {
          printf("%c", response[b]);
        }
        b++;
      }
      printf("\n");
      send(clientSock, response, 256, 0);
      free(response);
      //free(trimmed);
      sem_post(&sem);
    }
    else if(buffer[0] == 'A' && buffer[1] == 'V')
    {
      sem_wait(&sem);
      char **strings;
      char *response = calloc(256, 1);
      int tokens = makeargv(buffer, ";", &strings);
      char* trimmed = calloc(20, 1);
      trimmed = trimwhitespace(strings[1]);
      int a = 0;
      int validRegion = 0;
      while(n[a].name[0] != 0)
      {
        if(strcmp(n[a].name, trimmed) == 0)
        {
          validRegion = 1;
          break;
        }

        a++;
      }
      if(validRegion == 0)
      {
        strcpy(response, "NR;");
        strcat(response, trimmed);
        strcat(response, "\0");
      }

      else
      {
        node_t* node = findnode(n, strings[1]);
        if(node->isLeafNode == 0)
        {
          strcpy(response, "NL;");
          strcat(response, node->name);
          strcat(response, "\0");
        }
        else if(node->pollsOpen == false || (node->pollsOpen == true && node->pollsClosed == true))
        {
          strcpy(response, "RC;");
          strcat(response, node->name);
          strcat(response, "\0");
        }
        else
        {
          addVotes(node, strings[2], n);
          strcpy(response, "SC;\0");
        }
      }
      printf("Sending response to client at %s:%d, ", inet_ntoa(clientAddress.sin_addr), (int) ntohs(clientAddress.sin_port));
      int b = 0;
      while(response[b] != '\0')
      {
        if(response[b] == ';')
        {
          if(response[b+1] == '\0')
          {
            printf(" %s", NULL);
            break;
          }
          else
          {
            printf(" ");
          }
        }
        else
        {
          printf("%c", response[b]);
        }
        b++;
      }
      printf("\n");
      send(clientSock, response, 256, 0);
      free(response);
      //free(trimmed);
      sem_post(&sem);
    }
    else if(buffer[0] == 'R' && buffer[1] == 'V')
    {
      sem_wait(&sem);
      char **strings;
      char *response = calloc(256, 1);
      int tokens = makeargv(buffer, ";", &strings);
      char* trimmed = calloc(20, 1);
      trimmed = trimwhitespace(strings[1]);
      int a = 0;
      int validRegion = 0;
      while(n[a].name[0] != 0)
      {
        if(strcmp(n[a].name, trimmed) == 0)
        {
          validRegion = 1;
          break;
        }
        a++;
      }
      if(validRegion == 0)
      {
        strcpy(response, "NR;");
        strcat(response, trimmed);
        strcat(response, "\0");
      }
      else
      {
        node_t* node = findnode(n, trimmed);
        if(node->isLeafNode == 0)
        {
          strcpy(response, "NL;");
          strcat(response, node->name);
          strcat(response, "\0");
        }
        else if(node->pollsOpen == false || (node->pollsOpen == true && node->pollsClosed == true))
        {
          strcpy(response, "RC;");
          strcat(response, node->name);
          strcat(response, "\0");
        }
        else
        {
          char *ok = calloc(20, 1);
          ok = removeVotes(node, strings[2], n);
          if(strcmp(ok, "OK") != 0)
          {
            strcpy(response, "IS;");
            strcat(response, ok);
            strcat(response, "\0");
          }
          else
          {
            strcpy(response, "SC;\0");
          }
        }
      }
      printf("Sending response to client at %s:%d, ", inet_ntoa(clientAddress.sin_addr), (int) ntohs(clientAddress.sin_port));
      int b = 0;
      while(response[b] != '\0')
      {
        if(response[b] == ';')
        {
          if(response[b+1] == '\0')
          {
            printf(" %s", NULL);
            break;
          }
          else
          {
            printf(" ");
          }
        }
        else
        {
          printf("%c", response[b]);
        }
        b++;
      }
      printf("\n");
      send(clientSock, response, 256, 0);
      free(response);
      //free(trimmed);
      sem_post(&sem);
    }
    else if(buffer[0] == 'C' && buffer[1] == 'P')
    {
      sem_wait(&sem);
      char **strings;
      char *response = calloc(256, 1);
      int tokens = makeargv(buffer, ";", &strings);
      char* trimmed = calloc(20, 1);
      trimmed = trimwhitespace(strings[1]);
      int a = 0;
      int validRegion = 0;
      while(n[a].name[0] != 0)
      {
        if(strcmp(n[a].name, trimmed) == 0)
        {
          validRegion = 1;
          break;
        }
        a++;
      }
      if(validRegion == 0)
      {
        strcpy(response, "NR;");
        strcat(response, trimmed);
        strcat(response, "\0");
      }
      else
      {
        node_t* node = findnode(n, trimmed);
        if(node->pollsClosed == true)
        {
          strcpy(response, "PF;");
          strcat(response, trimmed);
          strcat(response, " Closed\0");
        }
        else
        {
          //printf("Entering recursion\n");
          closePolls(n, node);
          //printf("Exited recursion\n");
          strcpy(response, "SC;\0");
        }
      }
      printf("Sending response to client at %s:%d, ", inet_ntoa(clientAddress.sin_addr), (int) ntohs(clientAddress.sin_port));
      int b = 0;
      while(response[b] != '\0')
      {
        if(response[b] == ';')
        {
          if(response[b+1] == '\0')
          {
            printf(" %s", NULL);
            break;
          }
          else
          {
            printf(" ");
          }
        }
        else
        {
          printf("%c", response[b]);
        }
        b++;
      }
      printf("\n");
      send(clientSock, response, 256, 0);
      free(response);
      sem_post(&sem);
    }
    else
    {
      char *response = calloc(256, 1);
      strcpy(response, "UC;");
      strcat(response, buffer);
      strcat(response, "\0");
      printf("Sending response to client at %s:%d, ", inet_ntoa(clientAddress.sin_addr), (int) ntohs(clientAddress.sin_port));
      int b = 0;
      while(response[b] != '\0')
      {
        if(response[b] == ';')
        {
          if(response[b+1] == '\0')
          {
            printf(" %s", NULL);
            break;
          }
          else
          {
            printf(" ");
          }
        }
        else
        {
          printf("%c", response[b]);
        }
        b++;
      }
      printf("\n");
      send(clientSock, response, 256, 0);
      free(response);
    }
    //free(buffer);
  }
  close(clientSock);
  printf("Closed connection with client at %s:%d\n", inet_ntoa(clientAddress.sin_addr), (int) ntohs(clientAddress.sin_port));
}

//Function returnWinner
//Takes in the root node and the command
//Makes sure the polls are closed. After this finds the candidate with
//the highest number of votes in the root node.
//Returns the winning candidate.
char* returnWinner(node_t* n, char* command)
{
  //printf("Returning Winner\n");
  int i = 0;
  char* response = calloc(256, sizeof(char));
  while(n[i].name[0] != 0)
  {
    if(n[i].pollsClosed == false)
    {
      strcpy(response, "RO;");
      strcat(response, n[i].name);
      strcat(response, "\0");
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
  strcat(winner, "\0");
  strcpy(response, "SC;Winner:");
  strcat(response, winner);
  return response;
}

//Function countVotes
//Taks in the root node and th command
//Verifies that the region in the command
//is a valid region. If valid, it will count
//up all the votes in the region, format them,
//and return a string conataining all the votes info.
char* countVotes(node_t* n, char* command)
{
  //printf("Counting Votes\n");
  char* response = calloc(256, sizeof(char));
  char **strings;
  int tokens = makeargv(command, ";", &strings);
  char* trimmed = calloc(20, 1);
  trimmed = trimwhitespace(strings[1]);
  int a = 0;
  int validRegion = 0;
  while(n[a].name[0] != 0)
  {
    if(strcmp(n[a].name, trimmed) == 0)
    {
      validRegion = 1;
      break;
    }
    a++;
  }
  if(validRegion == 0)
  {
    strcpy(response, "NR;");
    strcat(response, strings[1]);
    strcat(response, "\0");
    return response;
  }
  node_t* node = findnode(n, trimmed);
  if(node->Candidates[0][0] == 0)
  {
    //Return no votes error code.
    strcpy(response, "No votes\0");
    return response;
  }
  strcpy(response, "SC;");
  int i = 0;
  int first = 1;
  while(node->Candidates[i][0] != 0)
  {
    if(first == 1)
    {
      first = 0;
    }
    else
    {
      strcat(response, ",");
    }
    strcat(response, node->Candidates[i]);
    strcat(response, ":");
    char buf[3];
    sprintf(buf, "%d", node->CandidatesVotes[i]);
    strcat(response, buf);
    i++;
  }
  strcat(response, "\0");
  return response;
}

//Function openPolls
//Takes in the root node and the node to open its polls.
//Opens the poll in this node, and then recursivley calls itself
//on each child of this original node.
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

//Function addVotes
//Takes in the node to add votes to and the command line
//Goes through each candidate in the command line
//and checks if already in the node. If so, add the votes to
//this candidate. If not, add the candidate and the votes to a new
//spot in the nodes candidate variables. Operates recursivley up the tree of nodes.
void addVotes(node_t* n, char* command, node_t* root)
{
  char** strings;
  int tokens = makeargv(command, ",", &strings);
  int i;
  for(i = 0; i < tokens; i++)
  {
    char** strings2;
    int tokens2 = makeargv(strings[i], ":", &strings2);
    int match = 0;
    int j = 0;
    while(n->Candidates[j][0] != 0)
    {
      if(strcmp(n->Candidates[j], strings2[0]) == 0)
      {
        match = 1;
        break;
      }
      j++;
    }
    if(match == 1)
    {
      n->CandidatesVotes[j] += atoi(strings2[1]);
    }
    else
    {
        strcpy(n->Candidates[j], strings2[0]);
        n->CandidatesVotes[j] = atoi(strings2[1]);
    }
  }
  if(n->parentName[0] != 0)
  {
    node_t* parent = findnode(root, n->parentName);
    addVotes(parent, command, root);
  }
  return;
}

//Function removeVotes
//Operates similar to addVotes but will remove votes.
//If the candidate is not in the node or subtracting the votes
//would result in negative votes, returns a Illegal Subtraction error.
//Otherwise it returns an ok message indicating succesful removal
//of the votes. Operates recursivley up the tree of nodes.
char* removeVotes(node_t* n, char* command, node_t* root)
{
  char** strings;
  char* IS = calloc(20, 1);
  int tokens = makeargv(command, ",", &strings);
  int i;
  for(i = 0; i < tokens; i++)
  {
    char** strings2;
    int tokens2 = makeargv(strings[i], ":", &strings2);
    int match = 0;
    int j = 0;
    while(n->Candidates[j][0] != 0)
    {
      if(strcmp(n->Candidates[j], strings2[0]) == 0)
      {
        match = 1;
        break;
      }
      j++;
    }
    if(match == 1)
    {
      if(n->CandidatesVotes[j] < atoi(strings2[1]))
      {
        strcpy(IS, strings2[0]);
        return IS;
      }
    }
    else
    {
      strcpy(IS, strings2[0]);
      return IS;
    }
  }
  for(i = 0; i < tokens; i++)
  {
    char** strings2;
    int tokens2 = makeargv(strings[i], ":", &strings2);
    int j = 0;
    while(n->Candidates[j][0] != 0)
    {
      if(strcmp(n->Candidates[j], strings2[0]) == 0)
      {
        break;
      }
      j++;
    }
    n->CandidatesVotes[j] -= atoi(strings2[1]);
    if(n->CandidatesVotes[j] == 0)
    {
      free(n->Candidates[j]);
      n->Candidates[j] = calloc(256, 1);
    }
  }
  if(n->parentName[0] != 0)
  {
    node_t* parent = findnode(root, n->parentName);
    addVotes(parent, command, root);
  }
  char* ok = calloc(20, 1);
  ok = "OK";
  return ok;
}

//Function closePolls
//Taks in the node and the root node
//Closes the poll of the specific node
//Then recusivley closes the polls of each child of this node.
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

//Function main
//Calls DAGCreator to set up the DAG data structure
//Goes through each node and sets up the polls and other info
//Initializes the candidate info for each node, and the global semaphore
//Opens up the server socket and listens for client connections
//When a client connects, the server creates a thread and send the
//client socket to the server thread function. Then the server goes back to listening
//for new connections. This allows multi threading of the server.
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
    mainnodes[i].id = i;
    mainnodes[i].pollsOpen = false;
    mainnodes[i].pollsClosed = false;
    if(mainnodes[i].num_children == 0)
    {
      mainnodes[i].isLeafNode = 1;
    }
    else
    {
      mainnodes[i].isLeafNode = 0;
    }
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
