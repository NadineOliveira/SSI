// retirado diratamente de 
// https://www.binarytides.com/server-client-example-c-sockets-linux/
//, alterado para estar mais claro




/*
	C ECHO client example using sockets
*/
#define FUSE_USE_VERSION 31

//#include "/home/nadine/Transferências/libfuse-master/include/fuse.h"
#include<fuse.h>

#include<stdio.h>	//printf
#include<string.h>	//strlen
#include<sys/socket.h>	//socket
#include<arpa/inet.h>	//inet_addr
#include<unistd.h>	//write
#include <signal.h>
#include <errno.h>

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



/*
int tempo=0;

int flag=0;

void handler(){
	alarm(1);
	tempo++;
	if(tempo==5)
		flag=1;
} 
*/

int main(int argc , char *argv[]){
	int sock;
	struct sockaddr_in server;
	char message[1000] , server_reply[2000];
	
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
	while(1){
		while((read_size = recv(sock , server_reply , 2000 , 0)) > 0 ){
			printf("Aguardando mensagens do server \n");
			printf("%s", server_reply);
			
			if(*server_reply==*"Introduz o codigo enviado: \n"){
				printf("vais introduzir o codigo\n");
			}
			
			if( send(sock , message , strlen(message) , 0) < 0){
				puts("Send failed");
				return 1;
			}
			
			//Receive a reply from the server
			if( recv(sock , server_reply , 2000 , 0) < 0){
				puts("recv failed");
				break;
			}
		}
		//open(file)
		
	}
	
	close(sock);
	return 0;
}
