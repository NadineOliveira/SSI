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
  int j = rand()%5;

  printf("generated code: %d\n",rgn[j]);

  //printf("code of sequence chosen: %d, in pos %d of above list.\n",rgn[j],j+1);
  return rgn[j];
} 

/*

int main(void){
  genMultRandom(); 

  return 0; 
}

*/
