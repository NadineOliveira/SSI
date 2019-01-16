//teste para leitura de ficheiro

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char const *argv[]){
 
  int c;
  FILE *file;
  int size = 1000;
  char* path = (char*)malloc(sizeof(char)*size);

  if(argc >= 2 ){
    strcpy(path,argv[1]);
  }else{
    strcpy(path,"/home/iamtruth/mestrado/segurança em sistemas informáticos/TP3/contact_storage");
  }

  printf("%s\n",path);

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

  //reading from file
  char* cha = (char*)malloc(sizeof(char)*size);
  FILE *fptr = fopen(path,"r");

  if (fptr  == NULL){
       printf("Error! opening file");

       // Program exits if the file pointer returns NULL.
       exit(1);
   }

  fscanf(fptr,"%s\n", cha);

  //scanf("%*c");

  printf("Data from the file:\n%s", cha);
  fclose(fptr); 
  
  /*
  file = fopen(path, "r");
  if (file) {
      while ((c = getc(file)) != EOF){
          printf("%c\n",c);
          putchar(c);
      }

      fclose(file);
  }
  */
  return 0;

}

