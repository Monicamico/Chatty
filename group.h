
/** file group.h
 *  Monica Amico, Matricola: 516801
 *  Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
 *  originale degli autori. 
 */

#include "message.h"
#include "hash.h"
#include "config.h"
#include "ops.h"

typedef struct group {
    char groupName[MAX_NAME_LENGTH];
    char userGname[MAX_NAME_LENGTH];
    hash_t* htg;
    struct group* next;
} group;

extern group* g_head;
extern group* g_tail;

/**
 * @function print_group
 * @brief Stampa la lista dei gruppi
 */
void print_group();

/**
 * @function find_group
 * @brief Ricerca di un gruppo nella lista
 *
 * @param gname  nome del gruppo
 *
 * @return <=0 se non è presente
 *         0 se presente
 */
int find_group(char* gname);

/**
 * @function isin_group
 * @brief Ricerca di un utente nel gruppo
 *
 * @param gname    nome del gruppo
 * @param username nome dell' utente 
 * 
 * @return <=0 se non è presente
 * @return  0 se presente
 * 
 */
int isin_group(char* gname,char* username);

/**
 * @function group_connect
 * @brief connessione di un utente nei 
 *        gruppi di appartenenza
 *
 * @param username    nome dell'utente
 * @param fd          fd dell' utente 
 * 
 */
void group_connect(char* username, int fd);

/**
 * @function group_disconnect
 * @brief disconnessione di un utente nei 
 *        gruppi di appartenenza
 *
 * @param username    nome dell'utente 
 * 
 */
void group_disconnect(char* username);

/**
 * @function add_group
 * @brief aggiunge un gruppo alla lista
 *        automaticamente aggiunge al gruppo
 *        l'utente che ha richiesto l'operazione 
 *
 * @param username    nome dell'utente
 * @param fd          fd dell' utente 
 * @param gname       nome del gruppo
 * 
 * @return OP_OK in caso di successo
 * @return OP_FAIL altrimenti
 * 
 */
op_t add_group(char* gname,char* username,int fd);


/**
 * @function adduser_group
 * @brief aggiunge un utente al gruppo 
 *
 * @param username    nome dell'utente
 * @param fd          fd dell' utente 
 * @param gname       nome del gruppo
 * 
 * @return OP_OK in caso di successo
 * @return OP_FAIL altrimenti
 * 
 */
op_t adduser_group(char* gname,char* username,int fd);


/**
 * @function deleteuser_group
 * @brief elimina un utente dal gruppo 
 *
 * @param username    nome dell'utente
 * @param fd          fd dell' utente 
 * @param gname       nome del gruppo
 * 
 * @return OP_OK in caso di successo
 * @return OP_FAIL altrimenti
 * 
 */
op_t deleteuser_group(char* gname, char* username);

/**
 * @function deleteuser_allgroup
 * @brief elimina un utente da tutti i gruppi 
 *
 * @param username    nome dell'utente
 * 
 */
void deleteuser_allgroup(char* username);

/**
 * @function delete_group
 * @brief elimina un gruppo se l'utente
 *        che ha richiesto l'operazione
 *        ha creato il gruppo
 *
 * @param sender     nome dell'utente
 * @param gname      nome del gruppo
 */
op_t delete_group(char* gname,char* sender);


/**
 * @function fdonline_group
 * @brief crea un array contenente 
 *        i file descriptor degli utenti 
 *        che sono online all'interno del
 *        gruppo
 * 
 * @param gname      nome del gruppo
 * 
 * @return array di file descriptor
 * @return NULL nel caso in cui nessun 
 *         utente sia online
 */
int* fdonline_group(char* gname);

/**
 * @function notconn_group
 * @brief crea un array contenente 
 *        i nomi degli utenti 
 *        che sono offline nel gruppo
 * 
 * @param gname      nome del gruppo
 * 
 * @return array
 * @return NULL nel caso in cui nessun 
 *         utente sia offline
 */
char* notconn_group(char* gname,int* n);

/**
 * @function destroy_group
 * @brief distrugge la lista di gruppi
 */
void destroy_group();