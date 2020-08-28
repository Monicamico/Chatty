
/** file listfduser.c
 *  Monica Amico, Matricola: 516801
 *  Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
 *  originale degli autori. 
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "listfduser.h"
#include "config.h"
#include "pthread.h"

/*
typedef struct user_list {
    int fd;
    char name[MAX_NAME_LENGHT + 1];
    user_list *next;
} fd_user;

extern fd_user* fd_user_head;
extern fd_user* fd_user_head;

*/

//restituisce l'indice dove è stato inserito (fd,user)
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int insert_fduser(int fd,char* user){
    pthread_mutex_lock(&mutex);
    fd_user* nuovo;
    if ( (nuovo = malloc(sizeof(fd_user))) == NULL ){
        pthread_mutex_unlock(&mutex);
        return -1;
    }
    memset(nuovo, 0, sizeof(fd_user));
    
    nuovo -> fd = fd;
    memset(nuovo->name,0,sizeof(char)*(MAX_NAME_LENGTH+1));
    strncpy(nuovo->name, user,strlen(user));
    nuovo->next = NULL;
    if(fd_user_head == NULL) fd_user_head = nuovo;
    else {
        nuovo -> next = fd_user_head;
        fd_user_head = nuovo;
    }
    pthread_mutex_unlock(&mutex);
    return 0;
}

void extract_fduser(int fd,char** user){
    pthread_mutex_lock(&mutex);
    memset(*user,0,sizeof(char)*(MAX_NAME_LENGTH+1));
    fd_user* corr;
    fd_user* prec=NULL;
    corr = fd_user_head;
    while(corr!=NULL){
        if(corr->fd == fd) {
            strncpy(*user,corr->name,strlen(corr->name));
            if(prec == NULL) fd_user_head = corr -> next;
            else  prec -> next = corr -> next;
            free(corr);
            corr = NULL;
        }
        else {
            prec = corr;
            corr = corr->next;
        }
    }
    pthread_mutex_unlock(&mutex);
}

void destroy_fdusers(){
     pthread_mutex_lock(&mutex);
    if(fd_user_head == NULL){
        pthread_mutex_unlock(&mutex);
        return;
    }
    fd_user* curr;
    fd_user* prec = NULL;
    curr = fd_user_head;
    while(curr!=NULL){
        prec = curr;
        curr= curr->next;
        free(prec);
    }
    pthread_mutex_unlock(&mutex);
}