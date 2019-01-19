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
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>	
#include <signal.h>
#include"string_to_int.c" //conversão de strings para integers 
#include <pthread.h>

//isto é um crime contra o que nos ensinaram, mas enfim
char absolutePathToDb[FILENAME_MAX];
int randomCodeTest = -1;
int codeFromClient = -1;
int client_sock = -1;


int temporizador = -1; // temporizador
int verificado = -1; //a alterar se o código for verificado


#define SIZE 1000




//funções auxiliares

//client_example.c

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


//server_example.c


struct cliente
{
	char* nome;
	char* email;
}*CLI;

struct cliente *clientes;

int totalClientesBD = -1;
int clienteAtual = -1;


void carregaDB(){
	FILE *fp;
	char * line = NULL;
  size_t len = 0;
  ssize_t read;
  char* nome;
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
      nome = strtok(NULL,";");
      clientes[i].email = malloc(sizeof(char)*(strlen(nome)));
      strcpy(clientes[i].email,nome);
      i++;
  }
	totalClientesBD= i;
  fclose(fp);
  if (line)
      free(line);
}







//código fuse modificado

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





//thread para o temporizador
/*
void *threadproc(void *arg){
		temporizador = 30;
    while( (temporizador>0) && (verificado != 1) ){
				char* mensagem = "Faltam ";
				strcat(mensagem,temporizador);
				strcat(mensagem," segundos para colocar o código");
				write(client_sock,"faltam ",strlen(""));
        temporizador = temporizador-5;
        sleep(5);
    }
    return 0;
}

*/



static int xmp_open(const char *path, struct fuse_file_info *fi){

	int res;

	printf("open\n");


	//gerar código aleatório
	int randomCodeGenerated = genMultRandom();

	if(randomCodeTest != randomCodeGenerated){
		verificado = -1;

		randomCodeTest = randomCodeGenerated;

		//NOTA: no final temos de garantir que se falhar o valor de 
		//randomCodeTest volta a -1, de modo a garantir que o else não executa quando não devia

		//passo 1: aceder ao email do utilizador atual
    //variavel global clientes[clienteAtual].email


		//passo 2: mandar código para o email que obtivemos

	  //TODO: descomentar isto quando tivermos certeza que o email está a ser bem transmitido
		//sendMailToSomeoneWithACode(clientes[clienteAtual].email,randomCodeGenerated);
    
    //informar o cliente que já mandamos o código
		write(client_sock,"codigoEnviado",strlen("codigoEnviado"));

    //TODO: no final retirar o código deste print, deve ser obvio porque
		printf("Código enviado para%s\n%d\n",clientes[clienteAtual].email,randomCodeGenerated);

		//temporizador e verificador de código de cliente
		char client_message[2000];
		write(client_sock,"Insira o codigo tem 30 segundos:",strlen("Insira o codigo tem 30 segundos:"));

		while( recv(client_sock , client_message , 2000 , 0) > 0){
			puts(client_message);
			printf("recebemos o código\n");
			if(strstr(client_message,"timeout")==NULL)
				codeFromClient = atoi(client_message);
			break;
		}

		if(codeFromClient == randomCodeGenerated){
			write(
				client_sock,
				"validadoCodigo",
				strlen("validadoCodigo")
			);
			verificado = 1;
			res = open(path, fi->flags);
			if (res == -1){ return -errno; }
			fi->fh = res;
			return 0;
		}else if(codeFromClient != -1){
			write(
				client_sock,
				"erradoCodigo",
				strlen("erradoCodigo")
			);
			codeFromClient = -1;


		}

		//Se chegamos aqui então concluimos que o tempo terminou
		//e como tal devemos dar erro e avisar o utilizador

    write(client_sock,"tempoEsgotado",strlen("tempoEsgotado"));

		randomCodeTest = -1;
		return -errno;

	}else{
		//neste caso devemos apenas abrir o ficheiro,
		//pois se chegamos aqui então é porque o código já foi enviado 
		//e verificado com sucesso

		if( verificado!=-1 ){
			res = open(path, fi->flags);
			if (res == -1){ return -errno; }
			fi->fh = res;
			return 0;	
		}


	}

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



//lida com a comunicação cliente->servidor
void *connection_handler(void *socket_desc){
	//Get the socket descriptor
	int sock = *(int*)socket_desc;
	int read_size;
	char client_message[2000];
	
	
	//Receive a message from client
	while( (read_size = recv(sock , client_message , 2000 , 0)) > 0 ){
		//como a única mensagem deve ser o código, convertemos
		//para inteiro a mensagem e colocamos numa variável global

		int codeFromUserConverted;
		str2int(&codeFromUserConverted,client_message,10);
		codeFromClient = codeFromUserConverted;
		
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





int main(int argc, char *argv[]){
  char* username = (char*)malloc(sizeof(char)*SIZE);

	getcwd(absolutePathToDb,FILENAME_MAX);
	printf("Current directory:%s\n",absolutePathToDb);

  //carregar os utilizadores 
  carregaDB();



	int socket_desc , c , *new_sock;
	struct sockaddr_in server , client;

	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1){
		printf("Could not create socket");
	}
	puts("Socket created");

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( 8888 );

	if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0){
		//print the error message
		perror("bind failed. Error");
		return 1;
	}
	puts("bind done");

	listen(socket_desc , 3);

	puts("Waiting for client to start");
	c = sizeof(struct sockaddr_in);

	client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);

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


	printf("\n\nNOTA:a consola na nova pasta não vai diretamente para a diretoria que vai ser montada\n");
	printf("Isto é porque se fosse seria necessário recarregar a pasta ou sair e voltar a entrar nela\n");
	printf("Como tal após colocar um utilizadr válido por favor faça cd ./teste para poder entrar no sistema de ficheiros criado\n\n");



	//inicializando nome do utilizador
	getLine("Introduza o seu utilizador> ",username, sizeof username);

	printf("%s\n",username);



	//verificar a existência do utilizador

	for(int z = 0;z < totalClientesBD;z++){
		printf("%s\n",clientes[z].nome);
  	if( strcmp(clientes[z].nome,username) == 0 ){ 
			clienteAtual = z; break;
		} 
	}
	if(clienteAtual == -1){
		//cliente não encontrado no base de dados
		write(
			client_sock,
			"O seu utilizador não foi encontrado na base de dados, logo não pode aceder ao sistema de ficheiros\n",
			strlen("O seu utilizador não foi encontrado na base de dados, logo não pode aceder ao sistema de ficheiros\n")
		);

		printf("nome não encontrado\n");

		close(socket_desc);

		return 0;
	}

	//debug
	printf("cliente conectado\n");
	write( 
		client_sock,
		"servidor reconheceu o nome, iniciando serviço\n",
		strlen("servidor reconheceu o nome, iniciando serviço\n")
	);

	printf("deverá ser redirecionado para a diretoria de entrada do sistema de ficheiros criado\n");
	printf("pode utilizar como se fosse o sistema normal, com a excepção da operação open\n");
	printf("esta foi modificada para apenas abrir ficheiros se obtiver um código de autorização\n");
	printf("este código será distribuido pelo contacto de email que está definido em contact_storage\n");
	printf("o código deve ser colocado na nova consola que foi criada quando tal for possivel\n");
	printf("note-se que apenas tem 30 segundos para colocar o código após ter sido distribuido\n\n");
	printf("quando quiser sair saia das consolas, volte à diretoria do make e chame 'make stop'\n\n");
	printf("se tiver algum problema entretanto poderá chamar 'make help' para obter informação de alguns problemas que podem ocorrer\n");



	//fechar o servidor, temporário
	//close(socket_desc);


  umask(0);
  return fuse_main(argc, argv, &xmp_oper, NULL);


}
