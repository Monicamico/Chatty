
/** file list.c
 *  Monica Amico, Matricola: 516801
 *  Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
 *  originale degli autori. 
 */

#include <stdlib.h>
#include <stdio.h>
#include "list.h"

int insert (int fd) {
	
	node * nuovo;
	if ( (nuovo = malloc(sizeof(node))) == NULL )
		return -1;
	
	nuovo -> fd = fd;
	if(fd_list_head == NULL && fd_list_tail == NULL) {
		fd_list_head = nuovo;
		fd_list_tail = nuovo;
		nuovo -> next = NULL;
	}
	else {
		fd_list_tail -> next = nuovo;
		fd_list_tail = nuovo;
		nuovo -> next = NULL;
	}
	return 0;
}

int extract(void) {
	int fd;
	if (fd_list_head == fd_list_tail) {
		fd = fd_list_head -> fd;
		free(fd_list_head);
		fd_list_head = NULL;
		fd_list_tail = NULL;
	}
	else {
		node * tmp = fd_list_head;
		fd_list_head = fd_list_head -> next;
		fd = tmp -> fd;
		tmp -> next = NULL;
		free(tmp);
	}
	return fd;
}

void print(node* fd_list_head) {
    while(fd_list_head != NULL) 
    {
        printf("[%d]-> ",fd_list_head -> fd);
        fd_list_head = fd_list_head -> next;
    }
    printf("\nFine\n");
}