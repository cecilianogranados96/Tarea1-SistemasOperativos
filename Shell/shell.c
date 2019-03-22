#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
typedef int bool;
#define true 1
#define false 0

bool conc = false;
int cur_pos = -1;
char *historial[10];
int cur_bufsize = 64;

/* Ejecutar el Shell */
int ejecutar_shell(char **args){
  pid_t pid, wpid;
  int status;
  pid = fork();
  if(pid == 0){ /* Proceso hijo */
    if (execvp(args[0], args) == -1){
      perror("lsh");
    }
    exit(EXIT_FAILURE);
  }else if (pid > 0){ /* Proceso padre */
    if (!conc){
      do{
        wpid =  waitpid(pid, &status, WUNTRACED);
      } while(!WIFEXITED(status) && !WIFSIGNALED(status));
    }
  }else{ /* Error en el proceso */
    perror("shell");
  }
  conc = false;
  return 1;
}

char **shell_split_line (char *line)
{
  char **tokens = malloc(cur_bufsize * sizeof(char*));
  int position = 0;
  char *token;
  token = strtok(line, " \t\r\n\a");
  while (token != NULL){
    tokens[position] = token;
    position++;
    if(position >= cur_bufsize){
      cur_bufsize += 64;
      tokens = realloc(tokens, cur_bufsize * sizeof(char*));
      if(!tokens){
        fprintf(stderr, "lsh: ERROR\n");
        exit(EXIT_FAILURE);
      }
    }
    token = strtok(NULL, " \t\r\n\a");
  }
  if ((position > 0) && (strcmp(tokens[position - 1], "&") == 0)){
    conc = true;
    tokens[position - 1] = NULL;
  }
  tokens[position] = NULL;
  return tokens;
} 

int shell_historial (char **args)
{
  if ((cur_pos == -1) || (historial[cur_pos] == NULL)){
    fprintf(stderr, "No hay comandos en el historial\n");
    exit(EXIT_FAILURE);
  }

  if (strcmp(args[0], "historial") == 0){
    int last_pos = 0, position = cur_pos, count = 0;
    if ((cur_pos != 10) && (historial[cur_pos + 1] != NULL)){
      last_pos = cur_pos + 1;
    }
    count = (cur_pos - last_pos + 10) % 10 + 1;
    while (count > 0){
      char *comando = historial[position];
      printf("%d %s\n", count, comando);
      position = position - 1;
      position = (position + 10) % 10;
      count --;
    }
  }else{
    char **cmd_args;
    char *comando;
    if (strcmp(args[0], "!!") == 0){
      comando = malloc(sizeof(historial[cur_pos]));
      strcat(comando, historial[cur_pos]);
      printf("%s\n", comando);
      cmd_args = shell_split_line(comando);
      int i;
      return ejecutar_shell(cmd_args);
    }else if (args[0][0] == '!'){
      if (args[0][1] == '\0'){
        fprintf(stderr, "Se espera un parametro antes de \"!\"\n");
        exit(EXIT_FAILURE);
      }
      /* EJECUTA EL COMANDO DE ACUERDO A LA POCISION */
      int offset = args[0][1] - '0';
      int next_pos = (cur_pos + 1) % 10;

      if ((next_pos != 0) && (historial[cur_pos + 1] != NULL)){
        offset = (cur_pos + offset) % 10;
      }else{
        offset--;
      }
      if (historial[offset] == NULL){
        fprintf(stderr, "No hay comandos en el historial\n");
        exit(EXIT_FAILURE);
      }
      comando = malloc(sizeof(historial[cur_pos]));
      strcat(comando, historial[offset]);
      cmd_args = shell_split_line(comando);
      int i;
      return ejecutar_shell(cmd_args);
    }
  }
}

int ejecutar_comando (char *line)
{
  int i;
  char **args = shell_split_line(line);
  if (args[0] == NULL){ /* COMANDO VACIO */
    return 1;
  }else if ((strcmp(args[0], "historial") == 0) || (strcmp(args[0], "!!") == 0) || (args[0][0] == '!')){
    return shell_historial(args);
  }
  cur_pos = (cur_pos + 1) % 10;
  historial[cur_pos] = malloc(cur_bufsize * sizeof(char));
  char **temp_args = args;
  int count=0;
  while (*temp_args != NULL){
    strcat(historial[cur_pos], *temp_args);
    strcat(historial[cur_pos], " ");
    temp_args++;
  }
  return ejecutar_shell(args);
}

char *leer_comando (void){
  cur_bufsize = 1024;
  int position = 0;
  char *buffer = malloc(sizeof(char) * cur_bufsize);
  int c;
  while(1){ 
    /* Leer el caracter */
    c = getchar();
    if (c == EOF || c == '\n'){
      buffer[position] = '\0';
      return buffer;
    }else{
      buffer[position] = c;
    }
    position++;
  }
}


void main(void){
  char *comando;
  int status;
  do{
    printf("Linea_Comandos>");
    comando = leer_comando();
    status = ejecutar_comando(comando);
    free(comando);
  } while (status);
}