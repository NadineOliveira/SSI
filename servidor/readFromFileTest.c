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
    //printf("%s", cha);
    p = strtok(cha, ";");
    if(p){ 
      //printf("%s\n",p); 
      strcpy(person,p);
    }
    p = strtok(NULL, ";");
    if(p){ 
      //printf("%s\n",p); 
      //strcpy(email,p);
    }

    if(strcmp(person, target) == 0){
      printf("%s matches %s\n",person,target);
      strcpy(email,p);
      break;
    }else{
      printf("%s doesn't match %s\n",person,target);
    }
  }

  fclose(fptr); 

  printf("name:%s\nemail:%s\n",person,email);

  return email;
}


int main(int argc, char const *argv[]){
 
  int c;
  char* path = (char*)malloc(sizeof(char)*size);

  if(argc >= 2 ){
    strcpy(path,argv[1]);
  }else{
    strcpy(path,"/home/iamtruth/mestrado/segurança em sistemas informáticos/TP3/contact_storage");
  }

  printf("%s\n",path);
  returnContactIfItMatches(path,"ExamplePerson");
  returnContactIfItMatches(path,"test");


  //reading from file
  /*
  char* cha = (char*)malloc(sizeof(char)*size);
  FILE *fptr = fopen(path,"r");

  char *p = (char*)malloc(sizeof(char)*size);
  char* person = (char*)malloc(sizeof(char)*size);
  char* email = (char*)malloc(sizeof(char)*size);

  char* target = (char*)malloc(sizeof(char)*size);
  strcpy(target,"ExamplePerson");

  if (fptr  == NULL){
       printf("Error! opening file");

       // Program exits if the file pointer returns NULL.
       exit(1);
   }

  while(fgets(cha, size, fptr) != NULL){
    printf("%s", cha);
    p = strtok(cha, ";");
    if(p){ 
      //printf("%s\n",p); 
      strcpy(person,p);
    }
    p = strtok(NULL, ";");
    if(p){ 
      //printf("%s\n",p); 
      strcpy(email,p);
    }

    printf("name:%s\nemail:%s\n",person,email);
    if(strcmp(person, target) == 0){
      printf("%s matches %s\n",person,target);
    }else{
      printf("%s doesn't match %s\n",person,target);
    }
  }

  printf("name:%s\nemail:%s\n",person,email);

  fscanf(fptr,"%s\n", cha);
  fclose(fptr); 
*/




  //writing to file

  /*
  FILE *f = fopen(path, "w");
  if (f == NULL)
  {
      printf("Error opening file!\n");
      exit(1);
  }

  /* print some text */
  /*
  const char *text = "Write this to the file";
  fprintf(f, "Some text: %s\n", text);

  /* print integers and floats */
  /*
  int i = 1;
  float py = 3.1415927;
  fprintf(f, "Integer: %d, float: %f\n", i, py);

  /* printing single chatacters */
  /*
  char ch = 'A';
  fprintf(f, "A character: %c\n", ch);

  fclose(f);

  */

  return 0;

}

