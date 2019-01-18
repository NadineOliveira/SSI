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

//isto é um crime contra o que nos ensinaram, mas enfim
char absolutePathToDb[FILENAME_MAX];
int randomCodeTest = -1;

//para o servidor
int sock;
struct sockaddr_in server;
char buff[50];



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
	write(sock , "open on fuse system\n" , strlen("open on fuse system\n"));

	int res;
	//gerar código aleatório
	int randomCodeGenerated = genMultRandom();
	


	if(randomCodeTest != randomCodeGenerated){
		int size = 1000,read_size;
		char* email = (char*)malloc(sizeof(char)*size);
		char* codeFromUser = (char*)malloc(sizeof(char)*size);


		//neste caso podemos fazer o pedido para tudo da autorização
		//i.e.: criar o código, enviar por email,
		//esperar por código de volta e se for o correto executar o open
		randomCodeTest = randomCodeGenerated;

		//NOTA: no final temos de garantir que se falhar o valor de 
		//randomCodeTest volta a 0,
		//de modo a garantir que o else não executa quando não devia


		//passo 1: aceder ao email do utilizador atual
		//isto vai ser feito por um pedido ao servidor
		//que deve retornar um email
		//vamos dizer que a mensagem é "emailToFuse"
		write(sock , "emailToFuse" , strlen("emailToFuse"));

		//depois esperamos por uma resposta
		if((read_size = recv(sock , email , size , 0)) > 0){
			printf("email recebido:%s\n",email);
		}else{
			//se não a recebermos, então algo correu muito mal
			//logo abortamos
			write(sock , "erroEmail" , strlen("erroEmail"));
			randomCodeTest = -1;
			return -errno;
		}

		//passo 2: mandar código para o email que obtivemos
		//relativamente simples
		//ficará comentado até estarmos confiantes que vai funcionar
		//por razões que devem ser relativamente obvias

	  //TODO: descomentar isto quando tivermos certeza que o email está a ser bem transmitido
		//sendMailToSomeoneWithACode(email,randomCodeGenerated);

		printf("%s\n%d\n",email,randomCodeGenerated);

		//passo 2.5: iniciar o temporizador aqui
		//efetivamente ele vai estar à espera até 30 segundos aqui,
		//retornando erro caso passe mais que isso ou o código
		//que o cliente meteu esteja errado


		//TODO: adicionar o temporizador aqui

		//honestamente não sei como fazer isto,
		//pesquisar depois

		//passo 3: obter código do utilizador que é passado ao servidor e depois para aqui
		//e comparar com o que temos
		if((read_size = recv(sock , codeFromUser , size , 0)) > 0){
			int codeFromUserConverted;
			str2int(&codeFromUserConverted,codeFromUser,10);
			if(codeFromUserConverted == randomCodeGenerated){
				//o código está certo e podemos continuar
				//nota: estamos a fazer printf porque se o códigoe está certo
				//ele apenas deve abrir o ficheiro como esperado
				//podemos depois alterar isto se se justificar
				printf("Code verified, opening file");

				res = open(path, fi->flags);
				if (res == -1){ return -errno; }
				fi->fh = res;
				return 0;

			}else{
				//se o código estiver errado devemos avisar e depois 
				//voltar a ficar à espera que os 30 segundos acabem

				//para tal devemos comunicar com o servidor a dar a mensagem que deve 
				//ser passada para o cliente
				write(sock , "codigoIncorreto" , strlen("codigoIncorreto"));		
			}
		}

		//TODO: adicionar o temporizador aqui
		//note-se que o if acima deverá estar entre um while
		//que nos conta o tempo a passar(ou algo semelhante)



		//Se chegamos aqui então concluimos que o tempo terminou
		//e como tal devemos dar erro e avisar o utilizador

		write(sock , "tempoEsgotado" , strlen("tempoEsgotado"));
		randomCodeTest = -1;
		return -errno;



	}else{
		//neste caso devemos apenas abrir o ficheiro,
		//pois se chegamos aqui então é porque o código já foi enviado 
		//e verificado com sucesso
		printf("---------------------test--------------------\n");
		res = open(path, fi->flags);
		if (res == -1){ return -errno; }
		fi->fh = res;
		return 0;
	}






	//coisas antigas abaixo, comentadas para o caso de precisarmos delas depois
	/*
	int size = 1000;

	char* pathToDB = (char*)malloc(sizeof(char)*size);
	char* target = (char*)malloc(sizeof(char)*size);

	//caminho para a base de dados
	//assume que está na mesma diretoria deste ficheiro
	strcpy(pathToDB,absolutePathToDb);
	strcat(pathToDB,"/contact_storage");

	//obter email a partir de nome
	strcpy(target,"test");
	char* email = getEmailFromFile(pathToDB,target);
	
	//mensagem de teste
	printf(
		"code:%d;;;;db:%s;;;;user:%s;;;;email:%s;;;;flags:%d;;;;path:%s\n",
		randomCodeGenerated,pathToDB,target,email,fi->flags,path
	);

	

	res = open(path, fi->flags);
	if (res == -1)
		return -errno;

	fi->fh = res;


	return 0;
	*/

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

int main(int argc, char *argv[])
{

	getcwd(absolutePathToDb,FILENAME_MAX);
	printf("Current directory:%s\n",absolutePathToDb);
	

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
		perror("connect to server failed. Error");
		printf("Make sure you have the server running before calling this program\n");
		return 1;
	}

	puts("Connected\n");

	//identificar como fuse
	strcpy(buff,"fuse");
	write(sock , buff , strlen(buff));
	

	
	
	umask(0);
	return fuse_main(argc, argv, &xmp_oper, NULL);
}
