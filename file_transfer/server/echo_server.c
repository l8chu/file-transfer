/* A simple echo server using TCP */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#define SERVER_TCP_PORT 3000	/* well-known port */
#define BUFLEN		256	/* buffer length */
#define ERRORMSG "error\n"
struct pdu 
{
	char type;
	int length;
	char data[BUFLEN];
};
struct pdu tpdu;//transfer pdu
struct pdu rpdu;//receive pdu
int fp; //for file opener      
int a; //for temporary    
char *directory = ".";
int ret;
int echod(int);
void reaper(int);
char file[BUFLEN];
FILE * fp1;
int main(int argc, char **argv)
{
	int 	sd, new_sd, client_len, port;
	struct	sockaddr_in server, client;
	switch(argc){
	case 1:
		port = SERVER_TCP_PORT;
		break;
	case 2:
		port = atoi(argv[1]);
		break;
	default:
		fprintf(stderr, "Usage: %d [port]\n", argv[0]);
		exit(1);
	}
	/* Create a stream socket	*/	
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "Can't creat a socket\n");
		exit(1);
	}
	/* Bind an address to the socket	*/
	bzero((char *)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(sd, (struct sockaddr *)&server, sizeof(server)) == -1){
		fprintf(stderr, "Can't bind name to socket\n");
		exit(1);
	}
	/* queue up to 5 connect requests  */
	listen(sd, 5);
	(void) signal(SIGCHLD, reaper);
	while(1) {
	  client_len = sizeof(client);
	  new_sd = accept(sd, (struct sockaddr *)&client, &client_len);
	  if(new_sd < 0){
	    fprintf(stderr, "Can't accept client \n");
	    exit(1);
	  }
	  switch (fork()){
	  case 0:		/* child */
		(void) close(sd);
		exit(echod(new_sd));
	  default:		/* parent */
		(void) close(new_sd);
		break;
	  case -1:
		fprintf(stderr, "fork: error\n");
	  }
	}
}

/*	echod program	*/
int echod(int sd)
{
  while(1){
	printf("Connected to client\n");
	for (a = 0; a < BUFLEN; a++)//set data to 100 limit
		{  
			rpdu.data[a] = '\0';
			tpdu.data[a] = '\0';
		}
	char* fs_name = "";
	char sdbuf[BUFLEN]; // Send buffer
	bzero(rpdu.data, BUFLEN);
	bzero(tpdu.data, BUFLEN);
	int n;
	while((n = recv(sd, (char*) &rpdu, BUFLEN, 0)) > 0){
	        printf("--- Recieved PDU Type:%c\n", rpdu.type);
		if (rpdu.type == 'D')
		{         
			//look for file name specify by client
			fprintf(stderr, "--- Searching file name: %s\n", rpdu.data);
			fp =  open(rpdu.data, O_RDONLY);     // open the file that the client requested 

			if (fp < 0)//If file doesn't exist which fp is negative, report a file error to client 
			{
				fprintf(stderr, " file is not there \n");
			
				strcpy(tpdu.data, ERRORMSG);
				tpdu.type = 'E';
				
				(void) write(sd, (char*) &tpdu, BUFLEN);
				//(void) sendto(s, (char*) &tpdu, 6, 0, (struct sockaddr *)&fsin, sizeof(fsin));
			}
			else //If file exists come into this branch
			{
				while ((n = read(fp, tpdu.data, BUFLEN)) > 0)//read data
				{					
						tpdu.length = n;
						tpdu.type = 'F';                       
						//send data to client
						fprintf(stderr, "%s\n", (char*) &tpdu);
						(void) write(sd, (char*) &tpdu, BUFLEN);											
				}
			}
		}
		if (rpdu.type == 'U') {
			tpdu.type = 'R';
			tpdu.length = 0;
			strcpy(tpdu.data, "");
			strcpy(file, rpdu.data);
			fp1 = fopen(file,"w");
			(void) send(sd, (char*) &tpdu, BUFLEN, 0);
			fprintf(stderr, "Sent PDU Type: %c\n", tpdu.type);
			(void) recv(sd, (char*) &rpdu, BUFLEN, 0);
			fprintf(stderr, "Recieved PDU Type: %c\n", rpdu.type);
			if (fp < 0)
           		 {
                		printf("Error writing file!\n");
            		}
            		else
           		 {	
				printf("Writing file %s\n", file);
                		fwrite((char*)rpdu.data,sizeof(char),rpdu.length,fp1);
                		fclose(fp1);
           		 }    
		}
		if (rpdu.type == 'L') {
			DIR * d;
			char * dir_name = rpdu.data;
			/* Open the current directory. */
			d = opendir (dir_name);
			if (! d) {
				fprintf (stderr, "Cannot open directory '%s': %s\n", dir_name, strerror (errno));
				exit (EXIT_FAILURE);
			}
			char dir[200];
			while (1) {
				struct dirent * entry;
				entry = readdir (d);
				if (! entry) 
					break;
				//printf ("%s\n", entry->d_name);
				strcat(tpdu.data, "\t\n");
				strcat(tpdu.data, entry->d_name);
			}
			tpdu.type = 'I';
			tpdu.length = 0;
			(void) send(sd, (char*) &tpdu, BUFLEN, 0);
			/* Close the directory. */
			if (closedir (d)) {
				fprintf (stderr, "Could not close '%s': %s\n", dir_name, strerror (errno));
				exit (EXIT_FAILURE);
			}
		}
		if (rpdu.type == 'P') {
			directory = rpdu.data;
			ret = chdir (directory);
			tpdu.type = 'R';
			tpdu.length = 0;
			strcpy(tpdu.data, "");
			(void) send(sd, (char*) &tpdu, BUFLEN, 0);
			fprintf(stderr, "Changed directory to: %s\n", directory);
		}	
	}
	close(sd);
	printf("[Server] Connection with Client closed. Server will wait now...\n");
	while(waitpid(-1, NULL, WNOHANG) > 0);
	close(sd);
	return(0);
  }
}
/*	reaper		*/
void	reaper(int sig)
{
	int	status;
	while(wait3(&status, WNOHANG, (struct rusage *)0) >= 0);
}
