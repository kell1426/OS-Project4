/*kell1426
*04/11/18
*Daniel Kelly
*4718021*/

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Structure for every node
typedef struct node{
	char name[1024];
	int isLeafNode;
	int num_children;
	char childName[50][1024];
	char parentName[1024];
	bool pollsOpen;
	bool pollsClosed;
	char** Candidates;
	int CandidatesVotes[100];
	int id;
}node_t;

// struct threadArgs{
// 	node_t* n;
// 	char *command;
// 	int socket;
// };

struct serverArgs{
	int socket;
	struct sockaddr_in clientAddress;
	node_t* n;
};

int makeargv(const char*s, const char *delimiters, char ***argvp){

	int error;
	int i;
	int numtokens;
	const char *snew;
	char *t;

	if ((s == NULL) || (delimiters == NULL) || (argvp == NULL)){

		errno = EINVAL;
		return -1;

	}

	*argvp = NULL; // already assigned as a new var, just blanking out

	snew = s + strspn(s, delimiters);

	if ((t = malloc(strlen(snew) + 1)) == NULL)
		return -1;

	strcpy(t, snew);

	numtokens = 0;

	if (strtok(t, delimiters) != NULL) // count number of tokens in s
		for (numtokens = 1; strtok(NULL, delimiters) != NULL; numtokens++);

	// create arg array for pointers to tokens
	if ((*argvp = malloc((numtokens + 1)*sizeof(char *))) == NULL){
		error = errno;
		free(t);
		errno = error;
		return -1;
	}

	// insert pointers to tokens into the arg array
	if (numtokens == 0)
		free(t);

	else{
		strcpy(t, snew);
		**argvp = strtok(t, delimiters);
		for(i = 1; i < numtokens; i++)
			*((*argvp) + i) = strtok(NULL, delimiters);
	}

	*((*argvp) + numtokens) = NULL; // put in final NULL pointer

	return numtokens;
}

node_t* findnode(node_t* start, char* tobefound){
	//Find the node in question
		node_t* temp = start;
		do {
			if( (strcmp(temp->name, tobefound)==0)){
				return temp;
			}

			temp++;
		} while(temp->name[0] != 0);
		return NULL;
}

void printgraph(node_t* n)
{
	int i = 0;
	while(n[i].name[0] != '\0')
	{
		printf("Node name is: %s\n", n[i].name);
		printf("Node's parent name is: %s\n", n[i].parentName);
		printf("Node has %d children\n", n[i].num_children);
		int j;
		for(j = 0; j < n[i].num_children; j++)
		{
			printf("Child %d is: %s\n", j, n[i].childName[j]);
		}
		i++;
	}
}

char *trimwhitespace(char *str)
{
  char *end;
  // Trim leading space
  while(isspace((unsigned char)*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;

  while(end > str && isspace((unsigned char)*end)) end--;

  // Write new null terminator
  *(end+1) = 0;

  return str;
}
