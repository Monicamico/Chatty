
/** file conncections.c
 *  Monica Amico, Matricola: 516801
 *  Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
 *  originale degli autori. 
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ops.h"
#include "hash.h"
#include "message.h"
#include "connections.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>


#ifndef CONNECTIONS_C_
#define CONNECTIONS_C_
#endif

#ifndef MAX_RETRIES
#define MAX_RETRIES     10
#endif

#ifndef MAX_SLEEPING
#define MAX_SLEEPING     3
#endif

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX  64
#endif

int openConnection(char* path, unsigned int ntimes, unsigned int secs){
	
	int fds;
	struct sockaddr_un sa;
	strncpy(sa.sun_path, path, UNIX_PATH_MAX);
	sa.sun_family=AF_UNIX;
	fds=socket(AF_UNIX,SOCK_STREAM,0);
	int i=0;
	while (i< ntimes)
	{
		connect(fds,(struct sockaddr*) &sa, sizeof(sa));

		if(errno==ENOENT)
			{i++; sleep(secs);}
		else
		{
			return fds;
		}
	}
	return -1;
}



int readn(long fd, void *buf, size_t size) {
    size_t left = size;
    int r=0;
    char *bufptr = (char*)buf;

    while(left>0) {
	if ((r=read((int)fd ,bufptr,left)) == -1) {
	    if (errno == EINTR) continue;
	    return -1;
	}
	if (r == 0) return 0;   // gestione chiusura socket
        left = left - r;
		bufptr = bufptr + r;
    }
    return size;
}

int writen(long fd, void *buf, size_t size) {
    size_t left = size;
    int r;
    char *bufptr = (char*)buf;
    while(left>0) {
	if ((r=write((int)fd ,bufptr,left)) == -1) {
	    if (errno == EINTR) continue;
	    return -1;
	}
	if (r == 0) return 0;  
    left    -= r;
	bufptr  += r;
    }
    return 1;
}
 

int sendData(long fd, message_data_t *msg){
	int r=0;

	r = write(fd, &(msg->hdr), sizeof(message_data_hdr_t));
	if(r <= 0) return -1;
 	r = writen(fd,msg->buf,msg -> hdr.len);
	if(r <= 0) return -1;
	return 1;
}

int sendHeader(long fd, message_hdr_t *msg){
	int r = 0;
	r = write (fd, msg, sizeof(message_data_hdr_t));
	if(r <= 0) return -1;
	return 1;
}

int sendRequest(long fd, message_t *msg){	
	int r = 0;
	r = sendHeader(fd, &msg->hdr);
	if(r <= 0) return -1;
	r = sendData(fd, &msg->data);
	if(r <= 0) return -1;
	return 1;
}

int readHeader(int connfd, message_hdr_t *hdr){
	memset(hdr, 0, sizeof(message_hdr_t));
	int r = read(connfd, hdr, sizeof(message_hdr_t));
	if(r <= 0) return -1;
	return 1;
}

int readData(int fd,message_data_t *data)
{
	int r = 0;
	memset(data, 0, sizeof(message_data_t));
	r = read (fd, &(data->hdr), sizeof(message_data_hdr_t));
	if(r <= 0) return -1;
	
	if(data->hdr.len > 0)	
	{
		data->buf = (char*) malloc(sizeof(char)*(data->hdr.len));
		memset(data->buf, 0, sizeof(char)*(data->hdr.len));
		r = readn (fd, data->buf, sizeof(char)*(data->hdr.len));
		if(r<=0) { 
			//free(data -> buf); 
			return -1;
		}
	}
	else data->buf = NULL;
	
	return 1;
}

int readMsg(int fd, message_t *msg) {
	memset(msg, 0, sizeof(message_t));
	int r = 0;
	r = readHeader(fd, &(msg->hdr));
	if(r <= 0) return -1;
	r = readData(fd, &(msg->data));
	if(r <= 0) return -1;
	return 1;
}

int readMsg_lock(int fd, pthread_mutex_t mux, message_t *msg) {
	pthread_mutex_lock(&mux);
	if(fd < 0 || msg == NULL) {
		return -1;
		pthread_mutex_unlock(&mux);
	}		
	int r = readMsg(fd, msg);
	pthread_mutex_unlock(&mux);
	return r;
}

int readHeader_lock(int connfd, pthread_mutex_t mux, message_hdr_t *hdr){
	int r=0;
	pthread_mutex_lock(&mux);
	r = readHeader(connfd,hdr);
	pthread_mutex_unlock(&mux);
	return r;
}

int readData_lock(int fd, pthread_mutex_t mux ,message_data_t *data)
{
	int r=0;
	
	pthread_mutex_lock(&mux);
	r = readData(fd, data);
	pthread_mutex_unlock(&mux);
	return r;
}

int sendData_lock(long fd, pthread_mutex_t mux, message_data_t *msg) {
	int r=0;
	pthread_mutex_lock(&mux);
	r = sendData(fd, msg);
	pthread_mutex_unlock(&mux);
	return r;	
}

int sendHeader_lock(long fd, pthread_mutex_t mux, message_hdr_t *msg){
	int r = 0;
	pthread_mutex_lock(&mux);
	r = sendHeader(fd, msg); 
	pthread_mutex_unlock(&mux);	
	return r;
}

int sendRequest_lock(long fd, pthread_mutex_t mux, message_t *msg){	
	int r = 0;
	pthread_mutex_lock(&mux);
	r = sendRequest(fd, msg);
	pthread_mutex_unlock(&mux);
	return r;
}
