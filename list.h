
/** file list.h
 *  Monica Amico, Matricola: 516801
 *  Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
 *  originale degli autori. 
 */

#ifndef list_h

typedef struct elem_list {
    int fd;
    struct elem_list* next;
} node;

#define list_h

/* lista di file descriptor */
extern node* fd_list_head; //puntatore alla testa
extern node* fd_list_tail; //puntatore alla coda

/**
 * @function insert
 * @brief inserisce in coda un fd
 *
 * @param fd  file descriptor
 * @return 0 in caso di successo
 *         -1 in caso di errore
 */
int insert(int fd);

/**
 * @function extract
 * @brief estrae un file descriptor 
 *        dalla testa della lista
 * 
 * @return fd
 *
 */
int extract(void);

/* stampa la lista */
void print(node* head); 
#endif 