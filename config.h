/*
 * chatterbox Progetto del corso di LSO 2017
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 */

 /** file config.h
 *  Monica Amico, Matricola: 516801
 *  Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
 *  originale degli autori. 
 */
/**
 * @file config.h
 * @brief File contenente alcune define con valori massimi utilizzabili
 */

#if !defined(CONFIG_H_)
#define CONFIG_H_

#define MAX_NAME_LENGTH      32

#define SOCKETNAME UnixPath

#define SYSCALL(r, c, e) \
	if ((r = c) == -1)   \
	{                    \
		perror(e);       \
	}

#define BUCKETS 200


typedef enum boolean { FALSE = 0, TRUE = 1 } bool_t;
extern int MaxHistMsgs;

// to avoid warnings like "ISO C forbids an empty translation unit"x
typedef int make_iso_compilers_happy;

#endif /* CONFIG_H_ */
