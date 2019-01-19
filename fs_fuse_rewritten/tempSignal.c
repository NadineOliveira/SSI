#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>

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

/*

int main(){

	signal(SIGALRM,handler);
	alarm(1);

	while(1){
		pause();
		if(flag==1){
			printf("timeout\n");
			break;
		}
		//lê input
	}

	return 0;
}

*/
