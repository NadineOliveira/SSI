// retirado diratamente de 
// https://www.binarytides.com/server-client-example-c-sockets-linux/
//, alterado para estar mais claro



/*
	C socket server example
*/

#include<stdio.h>
#include<string.h>	//strlen
#include<sys/socket.h>
#include<arpa/inet.h>	//inet_addr
#include<unistd.h>	//write
#include"sCalls.c"
#include"servidor/readFromFile.c"
#include"servidor/genRandomCode.c"
#include"servidor/sendEmail.c"


int main(int argc , char *argv[]){
	int socket_desc , client_sock , c , read_size;
	struct sockaddr_in server , client;
	char client_message[2000];
	
	//Create socket
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1){
		printf("Could not create socket");
	}
	puts("Socket created");
	
	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( 8888 );
	
	//Bind
	if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0){
		//print the error message
		perror("bind failed. Error");
		return 1;
	}
	puts("bind done");
	
	//Listen
	listen(socket_desc , 3);
	
	//Accept and incoming connection
	puts("Waiting for incoming connections...");
	c = sizeof(struct sockaddr_in);
	
	//accept connection from an incoming client
	client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
	if (client_sock < 0){
		perror("accept failed");
		return 1;
	}
	puts("Connection accepted");

	char* dir = "Introduza a diretoria do ficheiro: ";
	write(client_sock , dir , strlen(dir));
	int l;
	char* cod=NULL;
	char* file=NULL;
	struct fuse_file_info *fi = malloc(sizeof(struct fuse_file_info));
	//Receive a message from client

	while( (read_size = recv(client_sock , client_message , 2000 , 0)) > 0 ){
		//Send the message back to client_messaget
		printf("%s\n", client_message);
		dir = "Introduza o codigo enviado: ";
		
		if(cod==NULL)
			cod = strdup(client_message);
		else
			file = strdup(client_message);
		printf("enviou cliente\n");
		write(client_sock , dir , strlen(dir));
		/*if((cod!=NULL)&&(file!=NULL))
			Myopen(client_message,fi,cod);*/
		//write(client_sock , client_message , strlen(client_message));
		for(l=0;l<read_size * 10;l++){
			client_message[l] = ' ';
		}
	
	}
	
	
	if(read_size == 0){
		puts("Client disconnected");
		fflush(stdout);
	}else if(read_size == -1){
		perror("recv failed");
	}
	
	return 0;
}
