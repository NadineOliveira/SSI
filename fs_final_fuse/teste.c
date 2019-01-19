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
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <stdlib.h>


#define SIZE 1000

//efetivamentee a definir as teclas com o control pressionado
#define CONTROL(x) ((x) & 0x1F)

//isto é um crime contra o que nos ensinaram, mas enfim
char absolutePathToDb[FILENAME_MAX];

int randomCodeTest = -1;


//funções auxiliares

//client_example.c

#define WAIT 5
#define OKHERE       0
#define NO_INPUT 1
#define TOO_LONG 2


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
  	printf("erro na abertura do ficheiro de contactos\n");
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

	//printf("access\n");

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


	//printf("mknod\n");

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


	//printf("mkdir\n");

	res = mkdir(path, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_unlink(const char *path)
{
	int res;


	//printf("unlink\n");

	res = unlink(path);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_rmdir(const char *path)
{
	int res;


	//printf("rmdir\n");

	res = rmdir(path);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_symlink(const char *from, const char *to)
{
	int res;


	//printf("symlink\n");

	res = symlink(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_rename(const char *from, const char *to, unsigned int flags)
{
	int res;


	//printf("rename\n");

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


	//printf("link\n");

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


	//printf("chmod\n");

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


	//printf("chown\n");

	res = lchown(path, uid, gid);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_truncate(const char *path, off_t size,
			struct fuse_file_info *fi)
{
	int res;


	//printf("truncate\n");

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


	//printf("utimens\n");

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


	//printf("create\n");

	res = open(path, fi->flags, mode);
	if (res == -1)
		return -errno;

	fi->fh = res;
	return 0;
}




int tempo=0;
int flag=0;

void handler(){
	alarm(1);
	tempo++;
	if(tempo%5 == 0)
		printf("Passaram: %d segundos\n", tempo);
	if(tempo==30)
		flag=1;
} 



static int xmp_open(const char *path, struct fuse_file_info *fi){
	int res;

	//printf("open\n");

	//gerar código aleatório
	int randomCodeGenerated = genMultRandom();

	if(randomCodeTest != randomCodeGenerated){

		randomCodeTest = randomCodeGenerated;

	  //TODO: descomentar isto quando tivermos certeza que o email está a ser bem transmitido
		//sendMailToSomeoneWithACode(clientes[clienteAtual].email,randomCodeGenerated);
    
    //informar o cliente que já mandamos o código
    //TODO: no final retirar o código deste print, deve ser obvio porque
		printf("Código enviado para%s\n%d\n",clientes[clienteAtual].email,randomCodeGenerated);


		printf("Introduza o código:(tem 30 segundos)");
		
		int i = 0;
		int codigoColocado = -1; 

		signal(SIGALRM,handler);
		alarm(1);

		while(1){
			pause();
			if(flag==1){
				printf("timeout\n");
				break;
			}else{
				while ( (scanf("%d", &codigoColocado) != 1) && (flag!=1)) {
						clear_stream(stdin);
						printf("Invalid integer. Please try again: ");
						fflush(stdout);
				}
				if(flag == 1){
					break;
				}else{	

					if(codigoColocado == randomCodeGenerated){
						printf("codigo correto, continuando com o open");
						flag = 0;
						res = open(path, fi->flags);
						if (res == -1){ return -errno; }
						fi->fh = res;
						return 0;
					}else{
						printf("codigo errado, tente novamente\n");
					}

				}

			}
		}



		//Se chegamos aqui então concluimos que o tempo terminou
		//e como tal devemos dar erro e avisar o utilizador
		flag = 0;
		randomCodeTest = -1;
		return -errno;
		

	}else{
		//neste caso devemos apenas abrir o ficheiro,
		//pois se chegamos aqui então é porque o código já foi enviado 
		//e verificado com sucesso

		res = open(path, fi->flags);
		if (res == -1){ return -errno; }
		fi->fh = res;
		return 0;	


	}

	return 0;

}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
	int fd;
	int res;


	//printf("read\n");

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


	//printf("write\n");

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


	//printf("statfs\n");

	res = statvfs(path, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_release(const char *path, struct fuse_file_info *fi)
{

	//printf("release\n");

	(void) path;
	close(fi->fh);
	return 0;
}

static int xmp_fsync(const char *path, int isdatasync,
		     struct fuse_file_info *fi)
{


	//printf("fsynch\n");


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


	//printf("fallocate\n");

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


	//printf("setxattr\n");

	if (res == -1)
		return -errno;
	return 0;
}

static int xmp_getxattr(const char *path, const char *name, char *value,
			size_t size)
{
	int res = lgetxattr(path, name, value, size);


	//printf("getxattr\n");

	if (res == -1)
		return -errno;
	return res;
}

static int xmp_listxattr(const char *path, char *list, size_t size)
{
	int res = llistxattr(path, list, size);

	//printf("listxattr\n");

	if (res == -1)
		return -errno;
	return res;
}

static int xmp_removexattr(const char *path, const char *name)
{
	int res = lremovexattr(path, name);


	//printf("removexattr\n");

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


	//printf("copy_file_range\n");

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






int main(int argc, char *argv[]){
  char* username = (char*)malloc(sizeof(char)*SIZE);

	getcwd(absolutePathToDb,FILENAME_MAX);

  //carregar os utilizadores 
  carregaDB();


	printf("\n\nNOTA:a consola na nova pasta não vai diretamente para a diretoria que vai ser montada\n");
	printf("Isto é porque se fosse seria necessário recarregar a pasta ou sair e voltar a entrar nela\n");
	printf("Como tal após colocar um utilizadr válido por favor faça cd ./teste para poder entrar no sistema de ficheiros criado\n\n");


	//inicializando nome do utilizador
	printf("Introduza o seu utilizador\n");

	scanf("%s",username);

	printf("%s\n",username);



	//verificar a existência do utilizador

	for(int z = 0;z < totalClientesBD;z++){
		if( strcmp(clientes[z].nome,username) == 0 ){ 
			clienteAtual = z; break;
		} 
	}
	if(clienteAtual == -1){
		//cliente não encontrado no base de dados
		printf("nome não encontrado\n");
		return 0;
	}

	printf("sempre que abrir o open um ecra será criado para colocar o código\n");
	printf("se ele sair do ecra antes dos 30 segundos passarem então teve sucesso no open\n");
	printf("ele automaticamente sai depois de 30 segundos(e o open falhou)\n\n");

	printf("poderá utilizar o cd ./teste para entrar no sistema de ficheiros após este ecra desaparecer\n");
	printf("pode utilizar como se fosse o sistema normal, com a excepção da operação open\n");
	printf("apenas a operação open requer código de autorização\n");
	printf("este código será distribuido pelo contacto de email que está definido em contact_storage\n");
	printf("o código deve ser colocado no ecra quando lhe for pedido\n");

	printf("note-se que apenas tem 30 segundos para colocar o código após ter sido distribuido\n\n");
	printf("quando quiser sair saia das consolas, volte à diretoria do make e chame 'make stop'\n\n");
	printf("se tiver algum problema entretanto poderá chamar 'make help' para obter informação de alguns problemas que podem ocorrer\n");



	//iniciar fuse
  umask(0);
  return fuse_main(argc, argv, &xmp_oper, NULL);


}
