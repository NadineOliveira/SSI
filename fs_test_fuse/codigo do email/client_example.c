// retirado diratamente de 
// https://www.binarytides.com/server-client-example-c-sockets-linux/
//, alterado para estar mais claro




/*
	C ECHO client example using sockets
*/
#define FUSE_USE_VERSION 31

#include<fuse.h>

#include<stdio.h>	//printf
#include<string.h>	//strlen
#include<sys/socket.h>	//socket
#include<arpa/inet.h>	//inet_addr
#include<unistd.h>	//write
#include <signal.h>
#include <errno.h>

#define WAIT 5
#define WAIT 5
#define OK       0
#define NO_INPUT 1
#define TOO_LONG 2

static int getLine (char *prmpt, char *buff, size_t sz) {
    int ch, extra;

    // Get line with buffer overrun protection.
    if (prmpt != NULL) {
        printf ("%s", prmpt);
        fflush (stdout);
    }
    if (fgets (buff, sz, stdin) == NULL)
        return NO_INPUT;

    // If it was too long, there'll be no newline. In that case, we flush
    // to end of line so that excess doesn't affect the next call.
    if (buff[strlen(buff)-1] != '\n') {
        extra = 0;
        while (((ch = getchar()) != '\n') && (ch != EOF))
            extra = 1;
        return (extra == 1) ? TOO_LONG : OK;
    }

    // Otherwise remove newline and give string back to caller.
    buff[strlen(buff)-1] = '\0';
    return OK;
}

int main(int argc , char *argv[]){
	int sock;
	struct sockaddr_in server;
	char message[1000] , server_reply[2000];
	char* dir;
	char* nome;
	char buff[50];

	int res;
	
    char codigo[5] = {0} ; // in case of single character input
    fd_set input_set;
    struct timeval timeout;
    int ready_for_reading = 0;
    int read_bytes = 0;

    /* Empty the FD Set */
    FD_ZERO(&input_set );
    /* Listen to the input descriptor */
    FD_SET(0, &input_set);

    /* Waiting for some seconds */
    timeout.tv_sec = WAIT;    // WAIT seconds
    timeout.tv_usec = 0;    // 0 milliseconds

    /* Invitation for the user to write something */

    printf("Enter code: (in %d seconds)\n", WAIT);


	
	//Create socket
	sock = socket(AF_INET , SOCK_STREAM , 0);
	if (sock == -1){
		printf("Could not create socket");
	}
	puts("Socket created");
	


	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_family = AF_INET;
	server.sin_port = htons( 8888 );

	//Connect to remote server
	if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0){
		perror("connect failed. Error");
		return 1;
	}
	


	puts("Connected\n");


	
	int read_size;
	//keep communicating with server
	//printf("Introduza o nome de cliente: \n");
	getLine ("Introduza o nome de cliente> ", buff, sizeof(buff));
	write(sock , buff , strlen(buff));
	getLine ("Introduza a diretoria do ficheiro> ", buff, sizeof(buff));
	write(sock , buff , strlen(buff));
	while(1){

	    //while(1){
	    	/* Listening for input stream for any activity */
			printf("Introduza o codigo enviado: \n");
	    	ready_for_reading = select(1, &input_set, NULL, NULL, &timeout);
			
			if (ready_for_reading == -1) {
		        /* Some error has occured in input */
		        printf("Unable to read your input\n");
		        return -1;
		    } 
		    if(ready_for_reading){
		        read_bytes = read(0, codigo, 4);
		        if(codigo[read_bytes-1]=='\n'){
			        --read_bytes;
			        codigo[read_bytes]='\0';
		        }
		        if(read_bytes!=0){
		            printf("Read, %d bytes from input : %s \n", read_bytes, codigo);
					write(sock , codigo , strlen(codigo));
					break;
				}
				
		    }
		    else{
		       	printf(" %d Seconds are over - no data input \n", WAIT); 
		       	break;	
		    }

			//Receive a reply from the server
		if( recv(sock , server_reply , 2000 , 0) < 0){
			puts("recv failed");
			break;
		}
		//open(file)
		
		
		
		//puts("Server reply :");
		//puts(server_reply);
	}
	
	close(sock);
	return 0;
}
