
/** file group.c
 *  Monica Amico, Matricola: 516801
 *  Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
 *  originale degli autori. 
 */

#include "group.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

extern int MaxConnections;
extern int MaxHistMsgs;

pthread_mutex_t mutex_group = PTHREAD_MUTEX_INITIALIZER;

void print_group(){
    pthread_mutex_lock(&mutex_group);
    group* aus;
    aus = g_head;
    if(aus == NULL) {
        printf("Nessun Gruppo!");
         pthread_mutex_unlock(&mutex_group);
        return;

    }
    int i = 1;
    while(aus !=NULL){
        printf("______________________\n");
        printf("Gruppo %d: [%s]\n",i,aus->groupName);
        print_tab(aus->htg);
        printf("______________________\n");
        aus = aus->next;
        i++;
    }
     pthread_mutex_unlock(&mutex_group);
}

// 0 se il gruppo è presente nella lista
// -1 altrimenti 
int find_group(char* gname){
    pthread_mutex_lock(&mutex_group);
    group* aus;
    aus = g_head;
    while(aus!=NULL){
        if( strcmp(aus->groupName,gname) == 0){
             pthread_mutex_unlock(&mutex_group);
            return 0;
        } 
        aus = aus->next;
    }
    pthread_mutex_unlock(&mutex_group);
    return -1;
}

//0 se l'utente è presente nel gruppo altrimenti -1
int isin_group(char* gname,char* username){
    
    pthread_mutex_lock(&mutex_group);
    group* aus;
    aus = g_head;
    while(aus!=NULL){
        if(strcmp(gname,aus->groupName)==0){
            op_t esito = find_hash(aus->htg,username);
             pthread_mutex_unlock(&mutex_group);
            if(esito == OP_OK )
                return 0;
            else return -1;
        }
        aus = aus->next;
    }

     pthread_mutex_unlock(&mutex_group);
    return -1;
}

// aggiunge un gruppo alla lista, 
// l'amministratore username viene aggiunto al gruppo
op_t add_group(char* gname,char* username,int fd)
{
    pthread_mutex_lock(&mutex_group);
    group* new;
    if ( (new = malloc(sizeof(group))) == NULL )
        return OP_FAIL;

    strcpy(new->groupName,gname);
    strcpy(new->userGname,username);
    new->htg = createTable(20);  
    new->next = NULL;      
    insert_hash(new->htg,username,fd);

    if (g_head == NULL && g_tail == NULL) {
       g_head = new;
       g_tail = new;
    }
    else {
        group* aus;
        aus = g_head;
        while(aus != NULL)
        {
            if( strcmp(aus->groupName,gname) == 0){
                 hash_destroy(new->htg);
                 free(new);
                 pthread_mutex_unlock(&mutex_group);
                return OP_FAIL;
            }
            aus = aus->next;
        }
        g_tail -> next = new;
        g_tail = new;
        new -> next = NULL;        
    }
    pthread_mutex_unlock(&mutex_group);
    return OP_OK;
}

//aggiunge l'utente al gruppo gname
op_t adduser_group(char* gname,char* username,int fd)
{
    pthread_mutex_lock(&mutex_group);
    group* aus;
    aus = g_head;
    while (aus != NULL){
        if(strcmp(gname,aus->groupName)==0) break;
        aus = aus->next;
    }
    if (aus == NULL){
        pthread_mutex_unlock(&mutex_group);
        return OP_FAIL;
    }
    op_t op = insert_hash(aus->htg,username,fd);
    print_tab(aus->htg);
    pthread_mutex_unlock(&mutex_group);
    return op;
}

//connette all'interno del gruppo (in questo modo ho fd aggiornato)
void group_connect(char* user, int fd){
    pthread_mutex_lock(&mutex_group);
    group* aus;
    aus = g_head;
    while( aus != NULL ){
        hash_connect(aus->htg,user,fd);
        aus = aus->next;
    }
     pthread_mutex_unlock(&mutex_group);
}

//disconnette all'interno del gruppo 
void group_disconnect(char* user){
    pthread_mutex_lock(&mutex_group);
    group* aus;
    aus = g_head;
    while( aus != NULL ){
        hash_disconnect(aus->htg,user);
        aus = aus->next;
    }
     pthread_mutex_unlock(&mutex_group);
}

//cancella l'utente dal gruppo gname
op_t deleteuser_group(char* gname, char* username)
{
    group* aus;
    pthread_mutex_lock(&mutex_group);
    aus = g_head;
    while (aus != NULL){
        if(strcmp(gname,aus->groupName)==0) break;
        aus = aus->next;
    }
    if (aus == NULL){
         pthread_mutex_unlock(&mutex_group);
         return OP_FAIL;
    }
    op_t op = hash_delete(aus->htg,username);
    pthread_mutex_unlock(&mutex_group);
    return op;
}


//cancello l'utente da tutti i gruppi (utile nella deregistrazione)
void deleteuser_allgroup(char* username){

    pthread_mutex_lock(&mutex_group);
    group* aus;
    aus = g_head;
    while(aus !=NULL){
        hash_delete(aus->htg,username);
        aus = aus->next;
    }
    pthread_mutex_unlock(&mutex_group);
}


op_t delete_group(char* gname,char* sender)
{
    pthread_mutex_lock(&mutex_group);

    group* curr;
    group* prec = NULL;
    curr = g_head;
    while(curr!=NULL) {
         if(strcmp(gname,curr->groupName)==0) break;
         prec = curr;
         curr = curr->next;
    }
    if (curr == NULL){
        pthread_mutex_unlock(&mutex_group);
        return OP_FAIL;
    } 
  
    if (strcmp(curr->userGname,sender)== 0)
    {
         hash_destroy(curr->htg);
         prec->next = curr->next;
         free(curr);
         pthread_mutex_unlock(&mutex_group);
        return OP_OK;
   
    } else {
        pthread_mutex_unlock(&mutex_group);
        return OP_FAIL;
    }    
}

// restituisce array con gli fd degli utenti del gruppo che sono online
int* fdonline_group(char* gname)
{
    pthread_mutex_lock(&mutex_group);
    int* fds;
    group* aus;
    aus = g_head;
    while( aus!=NULL ){
        if(strcmp(gname, aus->groupName)== 0)
        {
            fds = fdonline_hash(aus->htg);
             pthread_mutex_unlock(&mutex_group);
            return fds;
        }
        aus = aus->next;
    }

    pthread_mutex_unlock(&mutex_group);
    return NULL;
}

char* notconn_group(char* gname,int* n){

    pthread_mutex_lock(&mutex_group);
    group* aus;
    aus = g_head;
    char* users;
     while( aus!=NULL ){

        if(strcmp(gname, aus->groupName)== 0) {
            users = notconnected_users(aus->htg,n);
            pthread_mutex_unlock(&mutex_group);
            return users;
           
        }
        aus = aus->next;
    }
    pthread_mutex_unlock(&mutex_group);
    return NULL;
}
void destroy_group(){
    pthread_mutex_lock(&mutex_group);
    group* curr;
    group* prec = NULL;
    curr = g_head;
    while(curr != NULL){
        hash_destroy(curr->htg);
        prec = curr;
        curr = curr->next;
        free(prec);
    }
     pthread_mutex_unlock(&mutex_group);
}