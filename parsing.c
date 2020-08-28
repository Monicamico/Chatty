
/** file parsing.c
 *  Monica Amico, Matricola: 516801
 *  Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
 *  originale degli autori. 
 */

#ifndef PARSING_H_
#define PARSING_H_

#include "parsing.h"
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

char * extractValue (char * st)
{
	int i = 0;
	while (st[i]!='=')
	{
		i++;
	}
	st=(st+i+2);
	
	i=strlen(st);
	while((st[i]==' ')||(st[i]=='	')||(st[i]=='\n')||(st[i]=='\0'))
	{
		i--;
	}
	st[i+1]='\0';
	 
	char * aux = NULL;
	aux=(char * )malloc((strlen(st)+1)*sizeof(char));
	aux[0]='\0';
	aux[strlen(st)]='\0';
	
	strncpy(aux,st,(strlen(st)));
	
	return aux;
}

int parse_config_file(char* filename, char *path, char *dir, char *statfile,
	int *maxconn, int *poolsize, int *msgsize, int *filesize, int *histsize){
		//apertura file config
	FILE *config = fopen(filename, "r");
	if ((config == NULL) || (errno == EINVAL))
		return -1;

	//controlli
	fseek(config, 0L, SEEK_END);
	if (errno != 0)
		return -1;
	long lung = 0;
	lung = ftell(config);
	if (errno != 0)
		return -1;
	rewind(config);

	//estraggo i dati + controllo
	char *aux = NULL;
	char *aux2 = NULL;
	aux = (char *)malloc(lung * sizeof(char));
	char *aux3;

	while (fgets(aux, lung, config))
	{
		if (aux == NULL)
			return -1;
		if (aux[0] != '#')
		{
			if (strncmp(aux, "UnixPath", 8) == 0) {
				aux3 = extractValue((aux + 8));
				strcpy(path, aux3);
				free(aux3);
				aux3 = NULL;
			}
			else if (strncmp(aux, "DirName", 7) == 0) {
				aux3 = extractValue((aux + 7));
				strcpy(dir, aux3);
				free(aux3);
				aux3 = NULL;
			}
			else if (strncmp(aux, "StatFileName", 12) == 0) {
				aux3 = extractValue((aux + 12));
				strcpy(statfile, aux3);
				free(aux3);
				aux3 = NULL;
			}
			else if (strncmp(aux, "MaxConnections", 14) == 0)
			{
				aux2 = extractValue((aux + 14));
				*maxconn = (int)strtol(aux2, NULL, 10);
				free(aux2);
				aux2 = NULL;
			}
			else if (strncmp(aux, "ThreadsInPool", 13) == 0)
			{
				aux2 = extractValue((aux + 13));
				*poolsize = (int)strtol(aux2, NULL, 10);
				free(aux2);
				aux2 = NULL;
			}
			else if (strncmp(aux, "MaxMsgSize", 10) == 0)
			{
				aux2 = extractValue((aux + 10));
				*msgsize = (int)strtol(aux2, NULL, 10);
				free(aux2);
				aux2 = NULL;
			}
			else if (strncmp(aux, "MaxFileSize", 11) == 0)
			{
				aux2 = extractValue((aux + 11));
				*filesize = (int)strtol(aux2, NULL, 10);
				free(aux2);
				aux2 = NULL;
			}
			else if (strncmp(aux, "MaxHistMsgs", 8) == 0)
			{
				aux2 = extractValue((aux + 8));
				*histsize = (int)strtol(aux2, NULL, 10);
				free(aux2);
				aux2 = NULL;
			}
		}
		aux[0] = '\0';
	}
	fclose(config);
	if(aux != NULL)
		free(aux);
	if(aux2 != NULL)
		free(aux2);
	if(aux3 != NULL)
		free(aux3);
	return 1;
}

#endif
