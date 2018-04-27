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

char* commandFormater(int command, char *line)
{
  char *buf = malloc(256);
  char **strings;
  int tokens;
  switch(command)
  {
    case 1:
      strcpy(buf, "RW; ;\0");
      break;
    case 2:
      tokens = makeargv(line, " ", &strings);
      strcpy(buf, "CV;");
      strcat(buf, strings[1]);
      strcat(buf, ";\0");
      break;
    case 3:
      tokens = makeargv(line, " ", &strings);
      strcpy(buf, "OP;");
      strcat(buf, strings[1]);
      strcat(buf, ";\0");
      break;
    case 4:
      tokens = makeargv(line, " ", &strings);
      strcpy(buf, "AV;");
      strcat(buf, strings[1]);
      strcat(buf, ";");
      //Add in votes info here
      break;
    case 5:
      tokens = makeargv(line, " ", &strings);
      strcpy(buf, "RV;");
      strcat(buf, strings[1]);
      strcat(buf, ";");
      //Add in votes info here
      break;
    case 6:
      tokens = makeargv(line, " ", &strings);
      strcpy(buf, "CP;");
      strcat(buf, strings[1]);
      strcat(buf, ";\0");
      break;
  }
  return buf;
}

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
    char *buf = malloc(256);
    while(fgets(buf, 255, commandFile) != NULL)
    {
      if(strcmp(buf, "\n") != 0)
      {
        char* p = strchr(buf, '\n');//Delete trailing \n character.
      	if(p)
      	{
      		*p = 0;
      	}
        int command = commandChecker(buf);
        char *formattedCommand = malloc(256);
        formattedCommand = commandFormater(command, buf);
        printf("%s\n", formattedCommand);
        free(formattedCommand);


      }
    }
    close(sock);
    printf("Closed connection with server at %s:%s\n", argv[2], argv[3]);
  }
  else
  {
    perror("Connection failed");
  }
}
