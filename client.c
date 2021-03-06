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

int commandChecker(char *buf);
char* commandFormater(int command, char *line);

//Function commandChecker
//Takes in one line from the command file
//and checks if any of the 6 commands are in the line
//returns an int identifier. If no command is in line
//returns 0 for the int identifier
int commandChecker(char *buf)
{
  if(strstr(buf, "Return_Winner") != NULL)
  {
    return 1;
  }
  else if(strstr(buf, "Count_Votes") != NULL)
  {
    return 2;
  }
  else if(strstr(buf, "Open_Polls") != NULL)
  {
    return 3;
  }
  else if(strstr(buf, "Add_Votes") != NULL)
  {
    return 4;
  }
  else if(strstr(buf, "Remove_Votes") != NULL)
  {
    return 5;
  }
  else if(strstr(buf, "Close_Polls") != NULL)
  {
    return 6;
  }
  else
  {
    return 0;
  }
}

//Function commandFormater
//Takes in the command identifier and the line from
//the command file. Formats the command into the correct
//format to be sent to the server.
char* commandFormater(int command, char *line)
{
  char *buf = malloc(256);
  char **strings;
  int tokens;
  int len = 0;
  int i = 0;
  switch(command)
  {
    case 0:
      strcpy(buf, line);
      strcat(buf, "\0");
      break;
    case 1:
      strcpy(buf, "RW;               ;\0");
      break;
    case 2:
      tokens = makeargv(line, " ", &strings);
      strcpy(buf, "CV;");
      strcat(buf, strings[1]);
      len = strlen(strings[1]);
      for(i = len; i < 15; i++)
      {
        strcat(buf, " ");
      }
      strcat(buf, "\0");
      break;
    case 3:
      tokens = makeargv(line, " ", &strings);
      strcpy(buf, "OP;");
      strcat(buf, strings[1]);
      len = strlen(strings[1]);
      for(i = len; i < 15; i++)
      {
        strcat(buf, " ");
      }
      strcat(buf, ";\0");
      break;
    case 4:
      tokens = makeargv(line, " ", &strings);
      strcpy(buf, "AV;");
      strcat(buf, strings[1]);
      len = strlen(strings[1]);
      for(i = len; i < 15; i++)
      {
        strcat(buf, " ");
      }
      strcat(buf, ";");
      break;
    case 5:
      tokens = makeargv(line, " ", &strings);
      strcpy(buf, "RV;");
      strcat(buf, strings[1]);
      len = strlen(strings[1]);
      for(i = len; i < 15; i++)
      {
        strcat(buf, " ");
      }
      strcat(buf, ";");
      break;
    case 6:
      tokens = makeargv(line, " ", &strings);
      strcpy(buf, "CP;");
      strcat(buf, strings[1]);
      len = strlen(strings[1]);
      for(i = len; i < 15; i++)
      {
        strcat(buf, " ");
      }
      strcat(buf, ";\0");
      break;
  }
  return buf;
}

//Function leafCounter
//If the command is add votes or remove votes
//then open the text file conataining the votes
//and store them into the Candidates and CandidatesVotes
//variables
void leafCounter(char* leafFile, char** Candidates, int CandidatesVotes[MAX_CANDIDATES])
{
  FILE *leaf = fopen(leafFile, "r");
  char *buf = malloc(256);
  while(fgets(buf, 256, leaf) != NULL)
  {
    if(strcmp(buf, "\n") != 0)
		{
			char* p = strchr(buf, '\n');//Delete trailing \n character.
		  if(p)
		  {
			  *p = 0;
		  }
      int match = 0;
      int i;
      for(i = 0; i < MAX_CANDIDATES; i++)
      {
        if(Candidates[i][0] == 0) //Candidate not in array
        {
          break;
        }
        else if(strcmp(Candidates[i], buf) == 0) //Match found
        {
          match = 1;
          break;
        }
      }
      if(match == 1)
      {
        CandidatesVotes[i]++;
      }
      else
	    {
				int j = 0;
				while(buf[j] != 0)
				{
					Candidates[i][j] = buf[j];	//Store this Candidate into array
					j++;
				}
	      CandidatesVotes[i]++;	//Increment this candidates total votes
	    }
    }
  }
  fclose(leaf);
  free(buf);
}

//Function main
//Open up a socket and connecto to the IP and port
//passed in from the arguments. Connect to the server using this
//socket. Open up the command file and process each command individually.
//Formats the printouts to STDOUT as well. Waits for the server
//to send the response before sending the next request.
int main(int argc, char **argv){
  struct node* mainnodes=(struct node*)malloc(sizeof(struct node)*MAX_NODES);

	if (argc != 4){
		printf("Usage: ./client <REQ FILE> <Server IP> <Server Port>\n");
		return -1;
	}

  int sock = socket(AF_INET, SOCK_STREAM, 0);

  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_port = htons(atoi(argv[3]));
  address.sin_addr.s_addr = inet_addr(argv[2]);

  if(connect(sock, (struct sockaddr *) &address, sizeof(address)) == 0)
  {
    printf("Initiated connection with server at %s:%s\n", argv[2], argv[3]);

    FILE *commandFile = fopen(argv[1], "r");
    char *buf = calloc(256, 1);
    while(fgets(buf, 256, commandFile) != NULL)
    {
      if(strcmp(buf, "\n") != 0)
      {
        char* p = strchr(buf, '\n');//Delete trailing \n character.
      	if(p)
      	{
      		*p = 0;
      	}
        int command = commandChecker(buf);
        char **Candidates = NULL;;
        int CandidatesVotes[MAX_CANDIDATES];
        if(command == 4 || command == 5)
        {
          //Candidates = malloc(MAX_CANDIDATES * sizeof(char*));
          Candidates = calloc(MAX_CANDIDATES, sizeof(char*));
          int i;
          for(i = 0; i < MAX_CANDIDATES; i++)
          {
            //Candidates[i] = malloc(256);
            Candidates[i] = calloc(256, sizeof(char));
          }
          for(i = 0; i < MAX_CANDIDATES; i++)
          {
            CandidatesVotes[i] = 0;
          }
          char **strings;
          int tokens = makeargv(buf, " ", &strings);
          leafCounter(strings[2], Candidates, CandidatesVotes);
        }
        char *formattedCommand = calloc(256, 1);
        formattedCommand = commandFormater(command, buf);
        if(command == 4 || command == 5)
        {
          int j = 0;
          while(Candidates[j][0] != 0)
          {
            if(CandidatesVotes[j] != 0)
            {
              if(j > 0)
              {
                strcat(formattedCommand, ",");
              }
              strcat(formattedCommand, Candidates[j]);
              strcat(formattedCommand, ":");
              char buffer[3];
              sprintf(buffer, "%d", CandidatesVotes[j]);
              strcat(formattedCommand, buffer);
            }
            j++;
          }
          strcat(formattedCommand, "\0");
        }
        char** strings3;
        int num = makeargv(formattedCommand, ";", &strings3);
        char *trimmed = calloc(20, 1);
        if(strings3[1] != NULL)
        {
          trimmed = trimwhitespace(strings3[1]);
        }
        if(strcmp(strings3[0], "RW") == 0)
        {
          printf("Sending request to server: RW %s %s\n", NULL, NULL);
        }
        else
        {
          printf("Sending request to server: %s %s %s\n", strings3[0], trimmed, strings3[2]);
        }
        send(sock, formattedCommand, 256, 0);
        char *response = calloc(256, 1);
        recv(sock, response, 256, 0);
        char** strings4;
        num = makeargv(response, ";", &strings4);
        printf("Received response from server: %s %s\n", strings4[0], strings4[1]);
        free(formattedCommand);
        if(command == 4 || command == 5)
        {
          int k;
          for(k = 0; k < MAX_CANDIDATES; k++)
          {
            free(Candidates[k]);
          }
          free(Candidates);
        }
      }
    }
    char *end = "END";
    send(sock, end, 3, 0);
    close(sock);
    printf("Closed connection with server at %s:%s\n", argv[2], argv[3]);
  }
  else
  {
    perror("Connection failed");
  }
}
