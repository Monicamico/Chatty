
/** file listfduser.h
 *  Monica Amico, Matricola: 516801
 *  Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
 *  originale degli autori. 
 */

#include "config.h"

typedef struct user_list {
    int fd;
    char name[MAX_NAME_LENGTH+1];
    struct user_list *next;
} fd_user;

extern fd_user* fd_user_head;

/* struttura per la corrispondenza fd-utenti */


/**
 * @function insert_fduser
 * @brief inserisce in testa il nuovo elemento
 * 
 * @param fd  file descriptor
 * @param user  nome utente
 *
 * @return 0 in caso di successo
 *         -1 in caso di errore
 */
int insert_fduser(int fd,char* user);


/**
 * @function extract_fduser
 * @brief estrae il nome dell'utente
 *        ed elimina dalla lista l'elemento
 *        con l'associazione fd-utente estratto 
 *
 * @param fd   file descriptore
 * @param user nome utente che verrà modificato
 *
 */
void extract_fduser(int fd, char**user);

/*distrugge la struttura */
void destroy_fdusers();