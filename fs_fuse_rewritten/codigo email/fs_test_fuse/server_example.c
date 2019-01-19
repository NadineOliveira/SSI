// retirado diratamente de 
// https://www.binarytides.com/server-client-example-c-sockets-linux/
//, alterado para estar mais claro



/*
	C socket server example
*/


//efetivamente é uma cópia do passthough que vai ser alterada
//o sistema deverá ser depois de compilado (gcc -Wall teste.c `pkg-config fuse3 --cflags --libs` -o teste)
//montado na diretoria teste na mesma pasta que este documento está em,
//usando ./teste ./testeFolder
//(note-se que deve ser alterado de sistema para sistema)

//nota: se quiseres com printf a funcionar, usar a opção -f nos argumentos
//./teste -f ./testeFolder
//atenção: isto vai bloquear a consola qjue estás a usar
//a razão deve ser obvia: para poder fazer os printfs nela
//outra vantagem: ele automáticamente desmonta o sistema quando terminas o comando

/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  Copyright (C) 2011       Sebastian Pipping <sebastian@pipping.org>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.
*/

/** @file
 *
 * This file system mirrors the existing file system hierarchy of the
 * system, starting at the root file system. This is implemented by
 * just "passing through" all requests to the corresponding user-space
 * libc functions. Its performance is terrible.
 *
 * Compile with
 *
 *     gcc -Wall teste.c `pkg-config fuse3 --cflags --libs` -o teste
 *
 * ## Source code ##
 * \include teste.c
 */


#define FUSE_USE_VERSION 31

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define _GNU_SOURCE

#ifdef linux
/* For pread()/pwrite()/utimensat() */
#define _XOPEN_SOURCE 700
#endif

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif


//includes para testar algumas coisas
#include"genRandomCode.c"
#include"readFromFile.c"
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>	
#include <signal.h>
#include"string_to_int.c" //conversão de d«strings para integers
#include"structCliente.c"

#include<stdlib.h>	//strlen
#include<unistd.h>	//write
#include<pthread.h> //for threading , link with lpthread
//#include"sendEmail.c"


//the thread function
void *connection_handler(void *);

struct cliente *clientes;


//para especificar quem o fuse e quem o cliente
int sockFuse = -1;

int sockClient = -1;
int totalClientesBD = -1;
int clienteAtual = -1;

#define OK       0
#define NO_INPUT 1
#define TOO_LONG 2

//isto é um crime contra o que nos ensinaram, mas enfim
char absolutePathToDb[FILENAME_MAX];
int randomCodeTest = -1;

//para o servidor

int sock;
struct sockaddr_in server;
char buff[50];
int totalinDB;

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
  clientes = (struct cliente*)malloc(N*sizeof(struct cliente));
  while ((read = getline(&line, &len, fp)) != -1) {
      //printf("Retrieved line of length %zu:\n", read);
      //printf("%s", line);
      if(i==N) N=N*2;
      nome = strtok(line,";");
      clientes[i].nome = malloc(sizeof(char)*(strlen(nome)));
      strcpy(clientes[i].nome,nome);
      printf("C: %s ", clientes[i].nome);
      nome = strtok(NULL,";");
      clientes[i].email = malloc(sizeof(char)*(strlen(nome)));
      strcpy(clientes[i].email,nome);
      i++;
  }
	totalinDB= i;
  fclose(fp);
  if (line)
      free(line);
}

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



static void *xmp_init(struct fuse_conn_info *conn,
		      struct fuse_config *cfg)
{
	(void) conn;
	cfg->use_ino = 1;

	/* Pick up changes from lower filesystem right away. This is
	   also necessary for better hardlink support. When the kernel
	   calls the unlink() handler, it does not know the inode of
	   the to-be-removed entry and can therefore not invalidate
	   the cache of the associated inode - resulting in an
	   incorrect st_nlink value being reported for any remaining
	   hardlinks to this inode. */
	cfg->entry_timeout = 0;
	cfg->attr_timeout = 0;
	cfg->negative_timeout = 0;

	return NULL;
}

static int xmp_getattr(const char *path, struct stat *stbuf,
		       struct fuse_file_info *fi)
{
	(void) fi;
	int res;

	//printf("getattr\n");

	res = lstat(path, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_access(const char *path, int mask)
{
	int res;

	printf("access\n");

	res = access(path, mask);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_readlink(const char *path, char *buf, size_t size)
{
	int res;

	//printf("readlink\n");

	res = readlink(path, buf, size - 1);
	if (res == -1)
		return -errno;

	buf[res] = '\0';
	return 0;
}


static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi,
		       enum fuse_readdir_flags flags)
{
	DIR *dp;
	struct dirent *de;

	(void) offset;
	(void) fi;
	(void) flags;

	//printf("readdir\n");

	dp = opendir(path);
	if (dp == NULL)
		return -errno;

	while ((de = readdir(dp)) != NULL) {
		struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = de->d_ino;
		st.st_mode = de->d_type << 12;
		if (filler(buf, de->d_name, &st, 0, 0))
			break;
	}

	closedir(dp);
	return 0;
}

static int xmp_mknod(const char *path, mode_t mode, dev_t rdev)
{
	int res;


	printf("mknod\n");

	/* On Linux this could just be 'mknod(path, mode, rdev)' but this
	   is more portable */
	if (S_ISREG(mode)) {
		res = open(path, O_CREAT | O_EXCL | O_WRONLY, mode);
		if (res >= 0)
			res = close(res);
	} else if (S_ISFIFO(mode))
		res = mkfifo(path, mode);
	else
		res = mknod(path, mode, rdev);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_mkdir(const char *path, mode_t mode)
{
	int res;


	printf("mkdir\n");

	res = mkdir(path, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_unlink(const char *path)
{
	int res;


	printf("unlink\n");

	res = unlink(path);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_rmdir(const char *path)
{
	int res;


	printf("rmdir\n");

	res = rmdir(path);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_symlink(const char *from, const char *to)
{
	int res;


	printf("symlink\n");

	res = symlink(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_rename(const char *from, const char *to, unsigned int flags)
{
	int res;


	printf("rename\n");

	if (flags)
		return -EINVAL;

	res = rename(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_link(const char *from, const char *to)
{
	int res;


	printf("link\n");

	res = link(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_chmod(const char *path, mode_t mode,
		     struct fuse_file_info *fi)
{
	(void) fi;
	int res;


	printf("chmod\n");

	res = chmod(path, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_chown(const char *path, uid_t uid, gid_t gid,
		     struct fuse_file_info *fi)
{
	(void) fi;
	int res;


	printf("chown\n");

	res = lchown(path, uid, gid);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_truncate(const char *path, off_t size,
			struct fuse_file_info *fi)
{
	int res;


	printf("truncate\n");

	if (fi != NULL)
		res = ftruncate(fi->fh, size);
	else
		res = truncate(path, size);
	if (res == -1)
		return -errno;

	return 0;
}

#ifdef HAVE_UTIMENSAT
static int xmp_utimens(const char *path, const struct timespec ts[2],
		       struct fuse_file_info *fi)
{
	(void) fi;
	int res;


	printf("utimens\n");

	/* don't use utime/utimes since they follow symlinks */
	res = utimensat(0, path, ts, AT_SYMLINK_NOFOLLOW);
	if (res == -1)
		return -errno;

	return 0;
}
#endif

static int xmp_create(const char *path, mode_t mode,
		      struct fuse_file_info *fi)
{
	int res;


	printf("create\n");

	res = open(path, fi->flags, mode);
	if (res == -1)
		return -errno;

	fi->fh = res;
	return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi){

	printf("open\n");
	int res;

				//o código está certo e podemos continuar
				//nota: estamos a fazer printf porque se o códigoe está certo
				//ele apenas deve abrir o ficheiro como esperado
				//podemos depois alterar isto se se justificar
				printf("Código verificado, abrindo o ficheiro\n");

				res = open(path, fi->flags);
				if (res == -1){ return -errno; }
				fi->fh = res;
				return 0;


}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
	int fd;
	int res;


	printf("read\n");

	if(fi == NULL)
		fd = open(path, O_RDONLY);
	else
		fd = fi->fh;
	
	if (fd == -1)
		return -errno;

	res = pread(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	if(fi == NULL)
		close(fd);
	return res;
}

static int xmp_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	int fd;
	int res;


	printf("write\n");

	(void) fi;
	if(fi == NULL)
		fd = open(path, O_WRONLY);
	else
		fd = fi->fh;
	
	if (fd == -1)
		return -errno;

	res = pwrite(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	if(fi == NULL)
		close(fd);
	return res;
}

static int xmp_statfs(const char *path, struct statvfs *stbuf)
{
	int res;


	printf("statfs\n");

	res = statvfs(path, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_release(const char *path, struct fuse_file_info *fi)
{

	printf("release\n");

	(void) path;
	close(fi->fh);
	return 0;
}

static int xmp_fsync(const char *path, int isdatasync,
		     struct fuse_file_info *fi)
{


	printf("fsynch\n");


	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */

	(void) path;
	(void) isdatasync;
	(void) fi;
	return 0;
}

#ifdef HAVE_POSIX_FALLOCATE
static int xmp_fallocate(const char *path, int mode,
			off_t offset, off_t length, struct fuse_file_info *fi)
{
	int fd;
	int res;

	(void) fi;


	printf("fallocate\n");

	if (mode)
		return -EOPNOTSUPP;

	if(fi == NULL)
		fd = open(path, O_WRONLY);
	else
		fd = fi->fh;
	
	if (fd == -1)
		return -errno;

	res = -posix_fallocate(fd, offset, length);

	if(fi == NULL)
		close(fd);
	return res;
}
#endif

#ifdef HAVE_SETXATTR
/* xattr operations are optional and can safely be left unimplemented */
static int xmp_setxattr(const char *path, const char *name, const char *value,
			size_t size, int flags)
{
	int res = lsetxattr(path, name, value, size, flags);


	printf("setxattr\n");

	if (res == -1)
		return -errno;
	return 0;
}

static int xmp_getxattr(const char *path, const char *name, char *value,
			size_t size)
{
	int res = lgetxattr(path, name, value, size);


	printf("getxattr\n");

	if (res == -1)
		return -errno;
	return res;
}

static int xmp_listxattr(const char *path, char *list, size_t size)
{
	int res = llistxattr(path, list, size);

	printf("listxattr\n");

	if (res == -1)
		return -errno;
	return res;
}

static int xmp_removexattr(const char *path, const char *name)
{
	int res = lremovexattr(path, name);


	printf("removexattr\n");

	if (res == -1)
		return -errno;
	return 0;
}
#endif /* HAVE_SETXATTR */

#ifdef HAVE_COPY_FILE_RANGE
static ssize_t xmp_copy_file_range(const char *path_in,
				   struct fuse_file_info *fi_in,
				   off_t offset_in, const char *path_out,
				   struct fuse_file_info *fi_out,
				   off_t offset_out, size_t len, int flags)
{
	int fd_in, fd_out;
	ssize_t res;


	printf("copy_file_range\n");

	if(fi_in == NULL)
		fd_in = open(path_in, O_RDONLY);
	else
		fd_in = fi_in->fh;

	if (fd_in == -1)
		return -errno;

	if(fi_out == NULL)
		fd_out = open(path_out, O_WRONLY);
	else
		fd_out = fi_out->fh;

	if (fd_out == -1) {
		close(fd_in);
		return -errno;
	}

	res = copy_file_range(fd_in, &offset_in, fd_out, &offset_out, len,
			      flags);
	if (res == -1)
		res = -errno;

	close(fd_in);
	close(fd_out);

	return res;
}
#endif

static struct fuse_operations xmp_oper = {
	.init           = xmp_init,
	.getattr	= xmp_getattr,
	.access		= xmp_access,
	.readlink	= xmp_readlink,
	.readdir	= xmp_readdir,
	.mknod		= xmp_mknod,
	.mkdir		= xmp_mkdir,
	.symlink	= xmp_symlink,
	.unlink		= xmp_unlink,
	.rmdir		= xmp_rmdir,
	.rename		= xmp_rename,
	.link		= xmp_link,
	.chmod		= xmp_chmod,
	.chown		= xmp_chown,
	.truncate	= xmp_truncate,
#ifdef HAVE_UTIMENSAT
	.utimens	= xmp_utimens,
#endif
	.open		= xmp_open,
	.create 	= xmp_create,
	.read		= xmp_read,
	.write		= xmp_write,
	.statfs		= xmp_statfs,
	.release	= xmp_release,
	.fsync		= xmp_fsync,
#ifdef HAVE_POSIX_FALLOCATE
	.fallocate	= xmp_fallocate,
#endif
#ifdef HAVE_SETXATTR
	.setxattr	= xmp_setxattr,
	.getxattr	= xmp_getxattr,
	.listxattr	= xmp_listxattr,
	.removexattr	= xmp_removexattr,
#endif
#ifdef HAVE_COPY_FILE_RANGE
	.copy_file_range = xmp_copy_file_range,
#endif
};

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


		//se ainda tivermos possibilidade de conectarmo-nos com alguém
		if( (sockClient ==-10) || (sockFuse == -1) ){
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
		}else{
			printf("alguém tentou-se juntar quando já tinhamos tanto cliente como fuse presente");
			write(
				client_sock,
				"fuse and client already present, so you won't be connected",
				strlen("fuse and client already present, so you won't be connected")
			);
		}

	}
	
	if (client_sock < 0)
	{
		perror("accept failed");
		return 1;
	}





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
	int input = 0;
	while( (read_size = recv(client_sock , client_message , 2000 , 0)) > 0 ){

		//menasgem de debug do servidor
		printf("Nova mensagem\n");
		//Send the message back to client_messaget
		printf("%s;;;;%d\n", client_message,read_size);


		//identificar o tipo de cliente
		//e atribuir valores especiais

		//buscar o email
		char* email=NULL;
		char* path=NULL;
		char* codigo=NULL;
		int randomCodeGenerated=0;
		if(input==1 && read_size>0){
			for(int i=0;i<totalinDB;i++){
				printf("for\n");
				if(strcmp(client_message,clientes[i].nome)==0){
					printf("entrou %s\n", clientes[i].nome);
					email = strdup(clientes[i].email);
					int randomCodeGenerated = genMultRandom();
					//envia codigo por email
				}
			}
		}
		if(input==2){
			//path
			printf("path\n");
			path = strdup(client_message);
		}
		if(input==3){
			//path
			printf("codigo\n");
			codigo = strdup(client_message);
			if(codigo == randomCodeGenerated){
				struct fuse_file_info *fi = malloc(sizeof(struct fuse_file_info));
				xmp_open(path, fi);//nao sei se é esta que é suposto invocar, ou a open diretamente
			}
			//verificar se o codigo gerado é igual e invocar a funçao open
			//mas o codigo é gerado na funçao open do fuse?????? estranho
			//deveria ser na main do fuse e so depois invocada a open que apenas abria o
		}

		input++;

		if((strcmp(client_message,"fuse") == 0) && (sockFuse == 0)){
			printf("received a fuse message with no fuse set, assuming it's the fuse client\n");
			sockFuse = client_sock;
		}
		if((strcmp(client_message,"cliente") == 0) && (sockClient == 0)){
			printf("received a client message with no client set, assuming it's the user client\n");
			sockClient = client_sock;
			write(client_sock,"you've been accepted as the client",strlen("you've been accepted as the client"));
		}

		//mensagens especiais pelo cliente
		//deverão ser a mensagem com o nome de utilizador
		// e com o código que lhe foi pedido

		//TODO:definir depois

		//TODO: quando dado o nome pesquisar por ele e não houver match desconectar o cliente

		//char* nome = ...
		// for(int z = 0;z < totalClientesBD;z++){
		//   if( strcpm(clientes[z],nome) == 0 ){ clienteAtual = z; break;} 
		// }
		// if(clienteAtual == -1){ 
		//	 write(
		//     client_sock,
		//     "your name does not exist in our database of contacts",
		//     strlen("your name does not exist in our database of contacts")
		//   )
		//   read_size = 0;
		//   break;
		//	 /*desconectar o cliente*/ 
		// }


		//TODO: determinar qual o formato que as mensagens de códigos devem ter entre o cliente e o servidor
		//if((strcmp(client_message,"............") == 0) && (client_sock == sockClient)){
		//	printf("received code:%d\n",client_message);
		//	if(sockFuse != -1){
		//		write(sockFuse,client_message,strlen(client_message));
		//	}else{
		//		//esta mensagem nunca deve ocorrer, pois tal significa que ele chegou
		//		//a um ponto onde consegue colocar um código sem ter o fuse a correr
		//		write(
		//			sockClient,
		//			"error: fuse not working, how the hell did you get till this point?",
		//			strlen("error: fuse not working, how the hell did you get till this point?")
		//		);
		//	}
		//}




		//tipos de mensagens especiais dadas pelo cliente fuse
		//para serem transmitidas ao cliente
		//são erros que podem ocorrer durante o processo de modo a avisar ao cliente que algo correu mal
		// tempoEsgotado ; codigoIncorreto ; erroEmail
		//ou um pedido do email do utilizador: emailToFuse

		if((strcmp(client_message,"emailToFuse") == 0) && (client_sock == sockFuse)){
			//efetivamente é o fuse a pedir pelo email do cliente atual
			write(client_sock,clientes[clienteAtual].email,strlen(clientes[clienteAtual].email));
		}
		

		if((strcmp(client_message,"tempoEsgotado") == 0) && (client_sock == sockFuse)){
			write(
				client_sock,
				"Acabou o tempo para colocar o código, open cancelado",
				strlen("Acabou o tempo para colocar o código, open cancelado")
			);
		}
		if((strcmp(client_message,"codigoIncorreto") == 0) && (client_sock == sockFuse)){
			write(
				client_sock,
				"Código introduzido incorreto, por fazor introduza o código que lhe foi dado para o seu email",
				strlen("Código introduzido incorreto, por fazor introduza o código que lhe foi dado para o seu email")
			);
		}
		if((strcmp(client_message,"erroEmail") == 0) && (client_sock == sockFuse)){
			write(
				client_sock,
				"Ocorreu um erro entre a comunicação do email entre servidor e o fuse, por favor volte a tentar o open",
				strlen("Ocorreu um erro entre a comunicação do email entre servidor e o fuse, por favor volte a tentar o open")
			);
		}

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

	//libertar o socket nas variáveis globais
	if(client_sock == sockClient){ sockClient = 0; clienteAtual = -1;	}
	if(client_sock == sockFuse) { sockFuse = 0; }
	
	return 0;
}
