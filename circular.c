
/** file circular.c
 *  Monica Amico, Matricola: 516801
 *  Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
 *  originale degli autori. 
 */

#include "circular.h"
#include <stdio.h>
#include <stdlib.h>

/*ricerca di un elemento, restituisce la lock associata a quell elemento*/
pthread_mutex_t* find(array_t* array, int x) {
    pthread_mutex_lock(&(array->array_lock));
    if(array->elements == NULL){
        pthread_mutex_unlock(&(array->array_lock));
        return NULL;
    }

    int i = 0;
    while(i < array->lenght) {
        if(array->elements[i] == x) {
            pthread_mutex_unlock(&(array->array_lock));
            return &(array->elements_lock[i]);
        }
        else i++;
    }
    pthread_mutex_unlock(&(array->array_lock));    
    return NULL;
}


/* restituisce array di file descriptor attivi */
int* array_elem(array_t* array, int* n)
{
    pthread_mutex_lock(&(array->array_lock));
    *n = array->n_of_elem;
    if (array->n_of_elem == 0) {
        pthread_mutex_unlock(&(array->array_lock));
        return NULL;
    }
    int* a = malloc(sizeof(int)* (array->n_of_elem));
    int k = 0, i = 0;
    while(i < array->lenght) {
        if(array->elements[i] != 0) {
            a[k] = array->elements[i];
            k++;
        }
        i++;
    }
    pthread_mutex_unlock(&(array->array_lock));    
    return a;
}

/*
Inserisce un elemento nell'array 
Restituisce -1 se non c'è spazio o in caso di errore (si può usare per contrllare MAXCONNECTIONS) 
oppure la posizione in cui è stato inserito l'elemento
*/
int insert_elem(array_t* array, int x){
    
    pthread_mutex_lock(&(array->array_lock));  
    if(array->elements == NULL){
        pthread_mutex_unlock(&(array->array_lock));
        return -1;
    }
    int i = 0; int first_zero = -1;
    while(i < array->lenght) {
        if(array->elements[i] == x) {
            pthread_mutex_unlock(&(array->array_lock));
            return i;
        }
        else if(array->elements[i] == 0 && first_zero == -1) {
            first_zero = i;
        }
        i++;
    }
    if(first_zero != -1) {
        array->elements[first_zero] = x;
        array->n_of_elem++;
        pthread_mutex_unlock(&(array->array_lock));
        return first_zero;
    }
    pthread_mutex_unlock(&(array->array_lock));    
    return -1;
}

/*cancellazione logica di un elemento, 
1 in caso di successo e 0 in caso di errore*/

int delete_elem(array_t* array, int x){

    pthread_mutex_lock(&(array->array_lock));
    if(array->elements == NULL){
        pthread_mutex_unlock(&(array->array_lock));
        return 0;
    }
    int i = 0;
    while(i < array->lenght) {
        if(array->elements[i] == x) {
            array->elements[i] = 0;
            array->n_of_elem--;
            pthread_mutex_unlock(&(array->array_lock));
            return 1;
        }
        else i++;
    }
    pthread_mutex_unlock(&(array->array_lock));    
    return 0;
}

array_t* create_circular(int size) {

    if(size == 0) return NULL;
    array_t* array = malloc(sizeof(array_t));
    array -> lenght = size;
    array -> n_of_elem = 0;
    array -> elements = calloc(size, sizeof(int));
    pthread_mutex_init(&(array -> array_lock),NULL);
    array -> elements_lock = malloc(sizeof(pthread_mutex_t)*size);
    for(int i = 0; i < size; i++)
        pthread_mutex_init(&(array -> elements_lock[i]),NULL);
    return array;
}

void destroy(array_t* array){

    if(array != NULL) {
        free(array->elements);
        for(int i = 0; i > array-> lenght; i++) {
            pthread_mutex_destroy(&(array -> elements_lock[i]));
        }
        pthread_mutex_destroy(&(array->array_lock));
        free(array->elements_lock);
        free(array);
    }
}

pthread_mutex_t * extract_at_index(array_t* array, int index) {
    pthread_mutex_lock(&(array->array_lock));
    if(index >= array->lenght || index < 0) {
        pthread_mutex_unlock(&(array->array_lock));    
        return NULL;
    }
    if(array -> elements[index] == 0) {
        pthread_mutex_unlock(&(array->array_lock));
        return NULL;
    } 
    pthread_mutex_unlock(&(array->array_lock));
    return &(array -> elements_lock[index]);
}