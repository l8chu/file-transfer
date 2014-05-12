/* A simple echo client using TCP */
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <string.h>
#define SERVER_TCP_PORT 3000    /* well-known port */
#define BUFLEN        256    /* buffer length */
#include <fcntl.h>
#define ERRORMSG "error\n"
int main(int argc, char **argv)
{
    struct PDU {
        char type;
        int length;
        char data[BUFLEN];
    } rpdu, spdu;
    int     n, i, bytes_to_read;
    int     sd, port, count_buf = 0;
    struct    hostent        *hp;
    struct    sockaddr_in server;
    char    *host, *bp, rbuf[BUFLEN], sbuf[BUFLEN], file[BUFLEN];
    char usres;
    FILE * fp;
    switch(argc){
    case 2:
        host = argv[1];
        port = SERVER_TCP_PORT;
        break;
    case 3:
        host = argv[1];
        port = atoi(argv[2]);
        break;
    default:
        fprintf(stderr, "Usage: %s host [port]\n", argv[0]);
        exit(1);
    }
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "Can't creat a socket\n");
        exit(1);
    }
    bzero((char *)&server, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    if (hp = gethostbyname(host)) 
      bcopy(hp->h_addr, (char *)&server.sin_addr, hp->h_length);
    else if ( inet_aton(host, (struct in_addr *) &server.sin_addr) ){
      fprintf(stderr, "Can't get server's address\n");
      exit(1);
    }
    /* Connecting to the server */
    if (connect(sd, (struct sockaddr *)&server, sizeof(server)) == -1){
      fprintf(stderr, "Can't connect \n");
      exit(1);
    }
    for(count_buf=0; count_buf<=BUFLEN; count_buf++){
        file[count_buf] = ' ';
    }   
    while(1){
        printf("Do you want to upload (U) /download (D) /change directory (P) /list directory (L)? \n");
        scanf(" %c", &usres);
        if (usres == 'D')
        {
            spdu.type = usres;
            printf("Please enter the filename that you want to download: ");
            scanf(" %s",file);
            spdu.length = sizeof(file);
            strcpy(spdu.data,file);
            //printf("%c",spdu.type);
            //printf("%s \n", spdu.data);
            (void) write(sd,(char*) &spdu,BUFLEN);
            n = recv(sd, (char*) &rpdu, BUFLEN, 0);
            if (rpdu.type == 'E')
            {
                printf("File Not Found!\n");
            }
            else
            {
                fp = fopen(file,"w");
                fwrite((char*)rpdu.data,sizeof(char),rpdu.length,fp);
                fclose(fp);
            }    
        }
        if (usres == 'U') {
            spdu.type = usres;
            printf("Please enter the file name to upload:");
            scanf ("%s", file);
            spdu.length = sizeof(file);
            strcpy(spdu.data,file);
            (void) write(sd,(char*) &spdu,BUFLEN);
            n = recv(sd, (char*) &rpdu, BUFLEN, 0);
            bzero((char*)&spdu.data,sizeof((char*)spdu.data));
            if (rpdu.type == 'R') {
                printf("Recieved PDU Type: %c\n",rpdu.type);
                fp = open (file, O_RDONLY);
		printf("Uploading %s\n", file);
                while ((n = read(fp, spdu.data, BUFLEN)) > 0) { //read data from 100bytes size file
                        spdu.length = n;
                        spdu.type = 'F';                       
                        printf("...\n", file);
                        (void) write(sd, (char*) &spdu, BUFLEN);                        
                }            
            }
         }
	if (usres == 'L') {
	    printf("Please enter the directory to list: ");
            scanf ("%s", file);
            spdu.length = sizeof(file);
            spdu.type = usres;
            strcpy(spdu.data, file);
		(void) send(sd, (char*) &spdu, BUFLEN, 0);
		(void) recv(sd, (char*) &rpdu, BUFLEN, 0);
		printf("--- Directory on server %s :\n", file);
		printf("%s\n", rpdu.data);
	}
	if (usres == 'P') {
	    printf("Please enter the directory to change to: ");
            scanf ("%s", file);
            spdu.length = sizeof(file);
            spdu.type = usres;
            strcpy(spdu.data, file);
		(void) send(sd, (char*) &spdu, BUFLEN, 0);
		(void) recv(sd, (char*) &rpdu, BUFLEN, 0);
		printf("--- Recieved PDU Type: %c\n", rpdu.type);
	}
	}    
    close(sd);
    return(0);
}
