#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

typedef int bool;

#define true 1
#define false 0

#define tamanioMaximoLinea    1024 // Tamaño máximo del comando
#define tamanioHistorial   10
#define tamanioTokenBuffer 64
#define delimitadorToken   " \t\r\n\a"

// variable para comprobar concurrencia entre proceso padre e hijo
bool concurrencia = false;
// variable para conocer el último comando ejecutado
int posicionUltimoComando = -1;
// variable para almacenar el historial de comandos
char *historial[tamanioHistorial];

int tamanoActualBuffer = tamanioTokenBuffer;

//-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
int ejecutarShell(char **args)
{
	pid_t pid, wpid;
	int estado;

	pid = fork();
	if(pid == 0)
	{ // Proceso hijo
		if (execvp(args[0], args) == -1)
		{
			perror("lsh");
		}
		exit(EXIT_FAILURE);
	}
	else if (pid > 0)
	{ //Proceso padre
		if (!concurrencia)
		{
			do
			{
				wpid =  waitpid(pid, &estado, WUNTRACED);
			} while(!WIFEXITED(estado) && !WIFSIGNALED(estado));
		}
	}
	else
	{ // Error en mientras realizaba forking
		perror("shell");
	}

	concurrencia = false;
	return 1;
}
//-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
char **lineaDivisionShell (char *linea)
{
	char **tokens = malloc(tamanoActualBuffer * sizeof(char*));
	int posicion = 0;
	char *token;

	token = strtok(linea, delimitadorToken);
	while (token != NULL)
	{
		tokens[posicion] = token;
		posicion++;

		if(posicion >= tamanoActualBuffer)
		{
			tamanoActualBuffer += tamanioTokenBuffer;
			tokens = realloc(tokens, tamanoActualBuffer * sizeof(char*));
			if(!tokens)
			{
				fprintf(stderr, "lsh: allocation error\n");
				exit(EXIT_FAILURE);
			}
		}

		token = strtok(NULL, delimitadorToken);
	}

	if ((posicion > 0) && (strcmp(tokens[posicion - 1], "&") == 0))
	{
		concurrencia = true;
		tokens[posicion - 1] = NULL;
	}

	tokens[posicion] = NULL;
	return tokens;
} 
//-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
int invocarHistorial (char **args)
{
	if ((posicionUltimoComando == -1) || (historial[posicionUltimoComando] == NULL))
	{
		fprintf(stderr, "No commands in history\n");
		exit(EXIT_FAILURE);
	}

	if (strcmp(args[0], "history") == 0)
	{
		int last_pos = 0, posicion = posicionUltimoComando, cantidad = 0;

		if ((posicionUltimoComando != tamanioHistorial) && (historial[posicionUltimoComando + 1] != NULL))
		{
			last_pos = posicionUltimoComando + 1;
		}

		cantidad = (posicionUltimoComando - last_pos + tamanioHistorial) % tamanioHistorial + 1;

		while (cantidad > 0)
		{
			char *comando = historial[posicion];
			printf("%d %s\n", cantidad, comando);
			posicion = posicion - 1;
			posicion = (posicion + tamanioHistorial) % tamanioHistorial;
			cantidad --;
		}
	}
	else
	{
		char **cmd_args;
		char *comando;
		if (strcmp(args[0], "!!") == 0)
		{
			comando = malloc(sizeof(historial[posicionUltimoComando]));
			strcat(comando, historial[posicionUltimoComando]);
			printf("%s\n", comando);
			cmd_args = lineaDivisionShell(comando);
			int i;

			return ejecutarShell(cmd_args);
		}
		else if (args[0][0] == '!')
		{
			if (args[0][1] == '\0')
			{
				fprintf(stderr, "Expected arguments for \"!\"\n");
				exit(EXIT_FAILURE);
			}
			//Posicion del comando a ejecutar
			int devolucion = args[0][1] - '0';
			int siguientePosicion = (posicionUltimoComando + 1) % tamanioHistorial;

			if ((siguientePosicion != 0) && (historial[posicionUltimoComando + 1] != NULL))
			{
				devolucion = (posicionUltimoComando + devolucion) % tamanioHistorial;
			}
			else
			{
				devolucion--;
			}

			if (historial[devolucion] == NULL)
			{
				fprintf(stderr, "No such command in history\n");
				exit(EXIT_FAILURE);
			}

			comando = malloc(sizeof(historial[posicionUltimoComando]));
			strcat(comando, historial[devolucion]);
			cmd_args = lineaDivisionShell(comando);
			int i;

			return ejecutarShell(cmd_args);
		}
		else
		{
			perror("shell");
		}
	}
}
//-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
int ejecutarComando (char *linea)
{
	int i;

	char **args = lineaDivisionShell(linea);

	if (args[0] == NULL)
	{ // cuando el usuario ingresa comando vacío
		return 1;
	}
	else if ((strcmp(args[0], "history") == 0) 
		  || (strcmp(args[0], "!!") == 0) 
		  || (args[0][0] == '!'))
	{
		return invocarHistorial(args);
	}

	posicionUltimoComando = (posicionUltimoComando + 1) % tamanioHistorial;
	historial[posicionUltimoComando] = malloc(tamanoActualBuffer * sizeof(char));
	char **ArgsTemporales = args;
	int cantidad=0;

	while (*ArgsTemporales != NULL)
	{
		strcat(historial[posicionUltimoComando], *ArgsTemporales);
		strcat(historial[posicionUltimoComando], " ");
		ArgsTemporales++;
	}

	if (posicionUltimoComando > 0)
	{
		printf("Inserted %s\n", historial[posicionUltimoComando-1]);
	}

	return ejecutarShell(args);
}
//-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
char *leerComando (void)
{
	tamanoActualBuffer = tamanioMaximoLinea;
	int posicion = 0;
	char *buffer = malloc(sizeof(char) * tamanoActualBuffer);
	int caracter;

	if (!buffer)
	{
		fprintf(stderr, "lsh: allocation error\n");
		exit(EXIT_FAILURE);
	}

	while(1)
	{
	// Lee un caracter
		caracter = getchar();

		if (caracter == EOF || caracter == '\n')
		{
			buffer[posicion] = '\0';
			return buffer;
		}
		else
		{
			buffer[posicion] = caracter;
		}
		posicion++;

		// Si se excede el buffer, se reasigna 
		if (posicion >= tamanoActualBuffer)
		{
			tamanoActualBuffer += tamanioMaximoLinea;
			buffer = realloc(buffer, tamanoActualBuffer);
			if (!buffer)
			{
				fprintf(stderr, "lsh: allocation error\n");
				exit(EXIT_FAILURE);
			}
		}
	}
}
//-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
void iniciarBucleShell (void)
{
	char *comando;
	int estado;

	do
	{
		printf(">");
		comando = leerComando();
		estado = ejecutarComando(comando);

		free(comando);
	} while (estado);
}
//-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
int main(void)
{
	iniciarBucleShell();
	return EXIT_SUCCESS;
}
