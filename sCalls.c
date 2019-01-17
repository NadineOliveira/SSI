#define FUSE_USE_VERSION 31
#define WAIT 30

//#include "/home/nadine/Transferências/libfuse-master/include/fuse.h"
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
//#include "sCalls.h"



static int Myopen(const char *path, struct fuse_file_info *fi, char* codigoCliente)
{
	int res;
	
    char codigo[10] = {0} ; // in case of single character input
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

    /* Listening for input stream for any activity */
    ready_for_reading = select(1, &input_set, NULL, NULL, &timeout);
    /* Here, first parameter is number of FDs in the set, 
     * second is our FD set for reading,
     * third is the FD set in which any write activity needs to updated,
     * which is not required in this case. 
     * Fourth is timeout
     */

    if (ready_for_reading == -1) {
        /* Some error has occured in input */
        printf("Unable to read your input\n");
        return -1;
    } 

    if (ready_for_reading) {
        read_bytes = read(0, codigo, 10);

        if(codigo[read_bytes-1]=='\n'){
	        --read_bytes;
	        codigo[read_bytes]='\0';
        }
        if(read_bytes!=0){
            printf("Read, %d bytes from input : %s \n", read_bytes, codigo);
            if(strcmp(codigo,codigoCliente)==0){

				res = open(path, fi->flags);
				if (res == -1)
					return -errno;

				fi->fh = res;
				return 0;

			}
			else{
				printf("Codigo errado\n");
				return -4;
			}
		}
		
    }
    else {
       	printf(" %d Seconds are over - no data input \n", WAIT);
    }
	
	return -2;
}
/*
int main(){
	struct fuse_file_info *fi = malloc(sizeof(struct fuse_file_info));
	int res = Myopen("/home/nadine/Documentos/SSI/SSI_TP3/contact_storage", fi, "1234");
	while(res==-4){
		res=Myopen("/home/nadine/Documentos/SSI/SSI_TP3/contact_storage", fi, "1234");
		printf("RES: %d\n", res);
		if((res==-2))
			break;
	}
	printf("RES: %d\n", res);
	return res;
}
*/
