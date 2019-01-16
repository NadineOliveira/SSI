//teste para leitura de ficheiro

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define size 1000


char* returnContactIfItMatches(char* pathToFile,char* recordToTest){

  char* cha = (char*)malloc(sizeof(char)*size);
  FILE *fptr = fopen(pathToFile,"r");  

  if (fptr  == NULL){
       printf("Error! opening file");
       // Program exits if the file pointer returns NULL.
       exit(1);
   }

  char *p = (char*)malloc(sizeof(char)*size);
  char* person = (char*)malloc(sizeof(char)*size);
  char* email = (char*)malloc(sizeof(char)*size);

  char* target = (char*)malloc(sizeof(char)*size);
  strcpy(target,recordToTest);

  while(fgets(cha, size, fptr) != NULL){
    p = strtok(cha, ";");
    if(p){ strcpy(person,p);}
    p = strtok(NULL, ";");
    if(strcmp(person, target) == 0){
      strcpy(email,p);
      break;
    }
  }

  fclose(fptr); 

  //printf("name:%s ; email:%s\n",person,email);


  return email;
}

//dado um caminho para ficheiro e dado uma pessoa,
//retorna o email correspondente
//atenção: retorna vazio se não tiver email associado a target

char* getEmailFromFile(char* path,char* target){
  char* email = (char*)malloc(sizeof(char)*size);
  strcpy(email, returnContactIfItMatches(path,target) );
  return(email);
}


int main(int argc, char const *argv[]){
 
  int c;
  char* path = (char*)malloc(sizeof(char)*size);
  char* target = (char*)malloc(sizeof(char)*size);

  //nota: assume que os utilizadores estão neste caminho
  
  strcpy(path,"../contact_storage");

  if(argc >= 2 ){
    strcpy(target,argv[1]);
  }else{
    strcpy(target,"ExamplePerson");
  }

  //printf("%s\n",path);
  char* email = getEmailFromFile(path,target);
  if(strcmp(email,"") == 0){
    printf("Target person not found\n");
  }else{
    printf("Target %s found with email %s",target,email);
  }

  return 0;

}

