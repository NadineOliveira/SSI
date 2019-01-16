//ver amis em https://www.geeksforgeeks.org/rand-and-srand-in-ccpp/

#include <stdio.h> 
#include <stdlib.h> 
#include<time.h> 
  
int createRandomCode(void){
  return(rand());
}

// Driver program 
int main(void){ 
  srand(time(0)); 
  for(int i = 0; i<5; i++) 
      printf(" %d\n", createRandomCode()); 

  return 0; 
} 
