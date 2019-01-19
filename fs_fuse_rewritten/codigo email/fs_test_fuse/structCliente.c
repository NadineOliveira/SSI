#include<stdio.h>
#include<string.h>	//strlen
#include<stdlib.h>	//strlen
struct cliente
{
	char* nome;
	char* email;
}*CLI;
/*
int carregaDB(struct cliente *clientes){
	FILE *fp;
	char * line = NULL;
  size_t len = 0;
  ssize_t read;
  char* nome;
  char* email;
  int N=10;
  int totalClientesBD = 0;
	//caminho para a base de dados
	//assume que está em contact_storage na mesma pasta que este ficheiro
  fp = fopen("./contact_storage", "r");
  if (fp == NULL){
  	printf("erro\n");
      exit(EXIT_FAILURE);
  }
  int i=0;
  while ((read = getline(&line, &len, fp)) != -1) {
      //printf("Retrieved line of length %zu:\n", read);
      //printf("%s", line);
      if(i==N) N=N*2;
      clientes = (struct cliente*)malloc(N*sizeof(struct cliente));
      nome = strtok(line,";");
      clientes[i].nome = malloc(sizeof(char)*(strlen(nome)));
      strcpy(clientes[i].nome,nome);
      nome = strtok(NULL,";");
      clientes[i].email = malloc(sizeof(char)*(strlen(nome)));
      strcpy(clientes[i].email,nome);
      i++;
  }
	totalClientesBD = i;
  fclose(fp);
  if (line)
      free(line);
  return totalClientesBD;
}*/