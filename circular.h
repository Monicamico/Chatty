
/** file circular.h
 *  Monica Amico, Matricola: 516801
 *  Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
 *  originale degli autori. 
 */

#ifndef circular_h
#define circular_h

#include <pthread.h>

typedef struct circulararray {
    int* elements;
    int lenght;
    int n_of_elem;
    pthread_mutex_t* elements_lock;
    pthread_mutex_t array_lock;
} array_t;

/* struttura contenente array di file descriptor e lock 
*  utile per l'invio di messaggi/file agli utenti
*/


/**
 * @function find
 * @brief Ricerca la lock associata a un fd
 *
 * @param array    puntatore all'array
 * @param x        file descriptor
 *
 * @return <=0 se non è presente
 *         0 se presente
 */
pthread_mutex_t* find(array_t* array, int x);


/**
 * @function insert_elem
 * @brief inserisce un fd se c'è spazio
 *
 * @param array    puntatore all'array
 * @param x        file descriptor
 *
 * @return <=0 in caso di errore altrimenti
 *         posizione dell'elemento inserito 
 */
int insert_elem(array_t* array, int x);


/**
 * @function delete_elem
 * @brief cancellazione logica di un elemento
 *
 * @param array    puntatore all'array
 * @param x        file descriptor
 *
 * @return 1 in caso di successo
 *         0 in caso di errore
 */
int delete_elem(array_t* array, int x);


/**
 * @function array_elem
 * @brief Ricerca file descriptor attivi
 *        e li inserisce in un array 
 * 
 * @param array    puntatore all'array
 * @param n        dimensione dell'array
 *
 * @return NULL se non è presente nessun fd attivo
 *         array di fd altrimenti 
 */
int* array_elem(array_t* array,int* n);


/**
 * @function create_circular
 * @brief crea la struttura di tipo
 *        array_t e alloca gli array 
 *        di dimensione size
 *
 * @param size    dimensione degli array
 *
 * @return la struttura creata 
 */
array_t* create_circular(int size);


/**
 * @function extract_at_index
 * @brief estrae la lock presente nela posizione
 *        passata come argomento
 *
 * @param array  struttura
 * @param index  posizione
 *
 * @return NULL in caso di errore altrimenti
 *         variabile mutua esclusione 
 */
pthread_mutex_t * extract_at_index(array_t* array, int index);


/* distrugge la strutture */
void destroy(array_t* array);

#endif