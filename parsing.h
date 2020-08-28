
/** file parsing.h
 *  Monica Amico, Matricola: 516801
 *  Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
 *  originale degli autori. 
 */

#ifndef PARSING_H_
#define PARSING_H_

char * extractValue (char * st);

int parse_config_file(char* filename, char *path, char *dir, char *statfile,
	int *maxconn, int *poolsize, int *msgsize, int *filesize, int *histsize);

#endif
