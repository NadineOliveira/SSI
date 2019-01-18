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
#include"servidor/readFromFile.c"
#include"servidor/genRandomCode.c"
#include"servidor/sendEmail.c"


//the thread function
void *connection_handler(void *);

struct cliente
{
	char* nome;
	char* email;
}*CLI;

struct cliente *clientes;

void carregaDB(){
	FILE *fp;
	char * line = NULL;
    size_t len = 0;
    ssize_t read;
    char* nome;
    char* email;
    int N=10;

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
	puts("Waiting for incoming connections...");
	c = sizeof(struct sockaddr_in);
	//accept connection from an incoming client
	puts("Waiting for incoming connections...");
	c = sizeof(struct sockaddr_in);
	while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) ){
		puts("Connection acceptedc");
		
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

		while( (read_size = recv(client_sock , client_message , 2000 , 0)) > 0 ){
			printf("NOva mensagem\n");
			//Send the message back to client_messaget
			printf("%s\n", client_message);
		}
	}
	
	if (client_sock < 0)
	{
		perror("accept failed");
		return 1;
	}

	//coisas a fazer depois da diretoria ter sido chamada:
	//1-gerar o código(através de genMultRandom() de getRandomCode.c)
	//2-enviar dito código para o email do utilzador
		//2.1- descobrir o email do utilziador(getEmailFromFile(path,target) de readFromFile)
		//2.2- mandar o email(sendMailToSomeoneWithACode(target,codeToSend) de sendEmail.c)

	
	//gerar código aleatório
	char* codigoGerado = genMultRandom();

	//caminho para a base de dados
	//nota: no produto final devemos decidir onde vai estar
	//mas por agora estará em contact_storage
	char* databaseForUsersEmails = "./contact_storage";

	

	//utilizador atual
	//nota: provavelmente vamos requirir que ele coloque o seu nome
	//ao conectar-se ao servidor, depois vemos se tem email associado
	//e caso não o tenha desconectar
	//por enquanto fica apenas a variável que depois temos de preencher
	char* userAtual;

	//obter o email do utilizador
	//vai ficar comentado até lermos o nome do utilizador
	//char* email = getEmailFromFile(databaseForUsersEmails,userAtual);

	//verificar se o email é vazio ou não
	//se for concluimos que o utilizador não está na base de dados
	//logo, desconectamos
	//mais tarde devemos usar sockets para evitar atirar o servidor abaixo
	//quando fazemos isto
	//if(strcmp(email,"") == 0){ printf("utilizador não encontrado"); exit(1);}

	//se tivermos o email, mandamos o código para ele e começamos a contar o tempo
	//provavelmente vamos ter de reformular o sCalls.c para isto funcionar
	//else{ 	
	//	sendMailToSomeoneWithACode(email,codigoGerado) 	
	//	.... //começar o temporizador do servidor 	
	//  .... //pedir código do utilizador e esperar por resposta
	//  .... //se ainda for antes dos 30 seguntos e o código for válido, abrir o ficheiro
	//  .... //caso contrário, desconectar utilizador
	//}


	//Receive a message from client

	
	
	if(read_size == 0){
		puts("Client disconnected");
		fflush(stdout);
	}else if(read_size == -1){
		perror("recv failed");
	}
	
	return 0;
}


/*
 * This will handle connection for each client
 * */
void *connection_handler(void *socket_desc){
	//Get the socket descriptor
	int sock = *(int*)socket_desc;
	int read_size;
	char *message , client_message[2000];
	
	//Send some messages to the client
	message = "Greetings! I am your connection handler\n";
	write(sock , message , strlen(message));
	
	message = "Now type something and i shall repeat what you type \n";
	write(sock , message , strlen(message));
	
	//Receive a message from client
	while( (read_size = recv(sock , client_message , 2000 , 0)) > 0 ){
		//Send the message back to client
		write(sock , client_message , strlen(client_message));
	}
	
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
