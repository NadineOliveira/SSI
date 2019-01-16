//ver amis em https://www.geeksforgeeks.org/rand-and-srand-in-ccpp/

#include <stdio.h> 
#include <stdlib.h> 
#include<time.h> 
  
int createRandomCode(void){
  return(rand());
}

int genMultRandom(void){ 
  int rgn[5];
  srand(time(0)); 
  for(int i = 0; i<5; i++){
    int code = createRandomCode();
    //printf(" %d\n",code); 
    rgn[i] = code;
  }
  int i = rand()%5;

  //printf("code of sequence chosen: %d, in pos %d of above list.\n",rgn[i],i+1);
  return rgn[i];
} 

int main(void){
  genMultRandom(); 

  return 0; 
}
