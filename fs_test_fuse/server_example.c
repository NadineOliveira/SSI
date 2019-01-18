// retirado diratamente de 
// https://www.binarytides.com/server-client-example-c-sockets-linux/
//, alterado para estar mais claro



/*
	C socket server example
*/

#include<stdio.h>
#include<string.h>	//strlen
#include<stdlib.h>	//strlen
#include<sys/socket.h>
#include<arpa/inet.h>	//inet_addr
#include<unistd.h>	//write
#include<pthread.h> //for threading , link with lpthread
#include"sCalls.c"
#include"readFromFile.c"
#include"genRandomCode.c"
#include"sendEmail.c"


//the thread function
void *connection_handler(void *);


struct cliente
{
	char* nome;
	char* email;
}*CLI;

struct cliente *clientes;

//para especificar quem o fuse e quem o cliente
int sockFuse = 0;
int sockClient = 0;



void carregaDB(){
	FILE *fp;
	char * line = NULL;
  size_t len = 0;
  ssize_t read;
  char* nome;
  char* email;
  int N=10;
	//caminho para a base de dados
	//assume que está em contact_storage na mesma pasta que este ficheiro
  fp = fopen("./contact_storage", "r");
  if (fp == NULL){
  	printf("erro\n");
      exit(EXIT_FAILURE);
  }
  int i=0;
  while ((read = getline(&line, &len, fp)) != -1) {
      //printf("Retrieved line of length %zu:\n", read);
      //printf("%s", line);
      if(i==N) N=N*2;
      clientes = (struct cliente*)malloc(N*sizeof(struct cliente));
      nome = strtok(line,";");
      clientes[i].nome = malloc(sizeof(char)*(strlen(nome)));
      strcpy(clientes[i].nome,nome);
      nome = strtok(NULL,";");
      clientes[i].email = malloc(sizeof(char)*(strlen(nome)));
      strcpy(clientes[i].email,nome);
      i++;
  }
  fclose(fp);
  if (line)
      free(line);
}


int main(int argc , char *argv[]){
	int socket_desc , client_sock , c , read_size, *new_sock;
	struct sockaddr_in server , client;
	char client_message[2000];
	carregaDB();
	
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
	//puts("Waiting for incoming connections...");
	//c = sizeof(struct sockaddr_in);
	
	//accept connection from an incoming client
	puts("Waiting for incoming connections...");
	c = sizeof(struct sockaddr_in);
	while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) ){
		puts("Connection accepted");
		
		pthread_t sniffer_thread;
		new_sock = malloc(1);
		*new_sock = client_sock;
		
		if( pthread_create( &sniffer_thread , NULL ,  connection_handler , (void*) new_sock) < 0){
			perror("could not create thread");
			return 1;
		}
		
		//Now join the thread , so that we dont terminate before the thread
		//pthread_join( sniffer_thread , NULL);
		puts("Handler assigned");


	}
	
	if (client_sock < 0)
	{
		perror("accept failed");
		return 1;
	}

	//nota: o utilizador atual deve ser inicializado no connection handler abaixo e não aqui
	//isto só fica aqui temporáriamente

	//utilizador atual
	char* userAtual;

	//email do utilizador
	char* email;






	//o comentado abaixo deve ser obsoleto, mas está assim para o caso de precisarmos de alguma coisa

	//Receive a message from client

	//while( (read_size = recv(client_sock , client_message , 2000 , 0)) > 0 ){
//
	//	//Send the message back to client_messaget
	//	printf("%s\n", client_message);
	//	dir = "Introduza o codigo enviado: ";
	//	
	//	if(cod==NULL)
	//		cod = strdup(client_message);
	//	else
	//		file = strdup(client_message);
//
	//	printf("enviou cliente\n");
	//	write(client_sock , dir , strlen(dir));
	//	
	//	if((cod!=NULL)&&(file!=NULL)){ Myopen(client_message,fi,cod); }
	//	
	//	//o abaixo é o suposto colocar-mos quando estiver a funcionar
	//	//if((cod!=NULL)&&(file!=NULL)){ Myopen(client_message,fi,codigoGerado); }
//
	//	
	//	write(client_sock , client_message , strlen(client_message));
	//	
	//	for(l=0;l<read_size * 10;l++){
	//		client_message[l] = ' ';
	//	}


}


/*
 * This will handle connection for each client
 * */
void *connection_handler(void *socket_desc){
	//Get the socket descriptor
	int client_sock = *(int*)socket_desc;
	int read_size;
	char *message , client_message[2000];
	
	//limpar o que estava antes em client_message
	//serve mais para garantir que as mensagens estão bem depois
	memset(client_message, 0, sizeof client_message);
	while( (read_size = recv(client_sock , client_message , 2000 , 0)) > 0 ){
		printf("Nova mensagem\n");
		//Send the message back to client_messaget
		printf("%s;;;;%d\n", client_message,read_size);

		//identificar o tipo de cliente
		//e atribuir valores especiais

		//TODO: ver se não faz mais sentido ter um handler especifico para cada tipo de cliente

		if((strcmp(client_message,"fuse") == 0) && (sockFuse == 0)){
			printf("received a fuse message with no fuse set, assuming it's the fuse client\n");
			sockFuse = client_sock;
		}
		if((strcmp(client_message,"cliente") == 0) && (sockClient == 0)){
			printf("received a client message with no client set, assuming it's the user client\n");
			sockClient = client_sock;
		}

		//mensagens especiais pelo cliente
		//deverão ser a mensagem com o nome de utilizador
		// e com o código que lhe foi pedido

		//TODO:definir depois



		//tipos de mensagens especiais dadas pelo cliente fuse
		//para serem transmitidas ao cliente
		//são erros que podem ocorrer durante o processo de modo a avisar ao cliente que algo correu mal
		// tempoEsgotado ; codigoIncorreto ; erroEmail

		//TODO:definir depois


		//limpar a mensagem depois de fazermos com ela o que queremos
		memset(client_message, 0, sizeof client_message);
	}



	//abaixo está o código original, obsoleto neste momento, ver acima

	/*
	//Send some messages to the client
	message = "Greetings! I am your connection handler\n";
	write(sock , message , strlen(message));
	
	message = "Now type something and i shall repeat what you type \n";
	write(sock , message , strlen(message));	

	//Receive a message from client
	while( (read_size = recv(client_sock , client_message , 2000 , 0)) > 0 ){
		//Send the message back to client
		write(client_sock , client_message , strlen(client_message));
	}
	*/

	if(read_size == 0){
		puts("Client disconnected");
		fflush(stdout);
	}else if(read_size == -1){
		perror("recv failed");
	}
		
	//Free the socket pointer
	free(socket_desc);
	
	return 0;
}
