/*
 * chatterbox Progetto del corso di LSO 2017
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 */
/** file chatty.c
 *  Monica Amico, Matricola: 516801
 *  Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
 *  originale degli autori. 
 */
/**
 * @file chatty.c
 * @brief File principale del server chatterbox
 */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/stat.h>
#include "list.h"
#include "connections.h"
#include "hash.h"
#include "message.h"
#include "ops.h"
#include "parsing.h"
#include "stats.h"
#include "circular.h"
#include "group.h"
#include "listfduser.h"
#include "config.h"
/*Variabili per la gestione dei file descriptor da cui
vengono lette le richieste ricevute dal server*/
node *fd_list_head;
node *fd_list_tail;

group* g_head;
group* g_tail;

fd_user* fd_user_head;

pthread_mutex_t lock_fd_list = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_fd_list = PTHREAD_COND_INITIALIZER;

/*"database" degli utenti connessi*/
hash_t *ht;

/*array utilizzato per la corrispondenza
  fd:lock */
array_t *fd_locks;

//configurazione del server
char UnixPath[UNIX_PATH_MAX];
char DirName[UNIX_PATH_MAX];
char StatFileName[UNIX_PATH_MAX];
int MaxConnections = 0;
int ThreadsInPool = 0;
int MaxMsgSize = 0;
int MaxFileSize = 0;
int MaxHistMsgs = 0;

// fd della socket
int fd_skt;

// Variabili ausiliarie per il funzionamento della select
fd_set aus;
int ausmax = 3;
pthread_mutex_t lock_fd_set = PTHREAD_MUTEX_INITIALIZER;

//struct statistiche 
struct statistics chattyStats = {0,0,0,0,0,0,0};
pthread_mutex_t lock_stats = PTHREAD_MUTEX_INITIALIZER;

//variabili per l'arresto con i segnali
bool_t stop = FALSE;
pthread_mutex_t mtx_stop = PTHREAD_MUTEX_INITIALIZER;

/*thread che accetta le connessioni e inserisce i fd nella coda*/
static void *listener();
/*threads che eseguono le richieste ricevute dal server*/
static void *worker();
/*thread handler dei segnali*/
static void * handler();

static inline void usage(const char *progname) {
	fprintf(stderr, "Il server va lanciato con il seguente comando:\n");
	fprintf(stderr, "  %s -f conffile\n", progname);
}

static inline int updatemax(fd_set set, int fdmax) {
    for(int i=(fdmax-1);i>=0;--i)
	if (FD_ISSET(i, &set)) return i;
    return -1;
}

int main(int argc, char *argv[]) {
	if ((argc != 3) || ((strncmp(argv[1], "-f", 2) != 0))) {
		usage(argv[0]);
		return 0;
	}
	/* Blocco i segnali che verranno gestiti dal thread handler */
	sigset_t set;
	memset(&set, 0, sizeof(sigset_t));
	if (sigemptyset(&set) != 0) { 
		perror("sigemptyset"); 
		exit(EXIT_FAILURE);
	}
	if ( sigaddset(&set, SIGINT) != 0 || sigaddset(&set, SIGQUIT) != 0
		|| sigaddset(&set, SIGTERM) != 0 || sigaddset(&set, SIGPIPE) != 0
		|| sigaddset(&set, SIGUSR1) != 0) { 
	
		perror("Sigaddset"); exit(EXIT_FAILURE);		
	} 
	pthread_sigmask(SIG_SETMASK, &set, NULL);
	
	
	//VALUTARE SE FARE REALLOC DELLE 3 STRINGHE
	if (parse_config_file(argv[2], UnixPath, DirName, StatFileName,
		&MaxConnections, &ThreadsInPool, &MaxMsgSize, &MaxFileSize, &MaxHistMsgs) == -1) {
			perror("parsing");
			exit(errno);
	}

	printf("UnixPath: %s\n",UnixPath);
	printf("MaxConnections: %d\n",MaxConnections);

	/* inizializzazione strutture dati 
		 Cartella Dirname
		 tabella hash
		 lista file descriptor
		 struttura gruppi
		 lista fd-utenti
		 lista circolare fd-mutex
	*/

	if (mkdir(DirName, 0777)== -1){
		if (errno == ENOSPC || errno == EROFS || errno == EACCES || errno == ENOTDIR) {
			perror("mkdir");
			exit(errno);
		}
	}

	ht = createTable(BUCKETS);
	
	//lista file descpriptor
	fd_list_head = NULL;
	fd_list_tail = NULL;

	//gruppi
	g_head = NULL;
	g_tail = NULL;
	
	//corrispondenza file descriptor utenti
	fd_user_head = NULL;
	
	//corrispondenza file descriptor mutex
	fd_locks = create_circular(MaxConnections);
	
	struct sockaddr_un sa;
	sa.sun_family = AF_UNIX;
	strncpy(sa.sun_path,UnixPath, 100);
	
	int r = 0;
	
	fd_skt = unlink(UnixPath);
	SYSCALL(fd_skt, socket(AF_UNIX, SOCK_STREAM, 0), "Socket");
	SYSCALL(r, bind(fd_skt, (struct sockaddr *)&sa, sizeof(sa)), "Bind");
	SYSCALL(r, listen(fd_skt, SOMAXCONN), "Listen");

	pthread_t l;
	pthread_t t[ThreadsInPool-2];
	pthread_t h;

	if (pthread_create(&l, NULL, &listener, NULL) == -1) {
		perror("errore creazione thread listener!");
		exit(EXIT_FAILURE);
	}

	if (pthread_create(&h, NULL, &handler, NULL) == -1) {
		perror("errore creazione thread handler!");
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i < ThreadsInPool-2; ++i) {
		if (pthread_create(&t[i], NULL, &worker, NULL) == -1) {
			perror("errore creazione thread worker!");
			exit(EXIT_FAILURE);
		}
	}

	printf("Server attivo!\n");

	pthread_join(l, NULL);
	for (int i = 0; i < ThreadsInPool-2; ++i)
		pthread_join(t[i], NULL);
	pthread_join(h, NULL);

	//Faccio pulizia
	hash_destroy(ht);
	destroy(fd_locks);
	destroy_fdusers();
	destroy_group();
	pthread_mutex_destroy(&lock_stats);
	pthread_mutex_destroy(&lock_fd_list);
	pthread_mutex_destroy(&lock_fd_set);

	return 0;
}

static void *listener() {
	/*Maschera per la select*/
	fd_set rdset;
	
	int fd_client = -1;

	memset(&aus, 0, sizeof(fd_set));
	memset(&rdset, 0, sizeof(fd_set));
	
	pthread_mutex_lock(&lock_fd_set);
	FD_SET(fd_skt, &aus); 
	int max_fd = ausmax;
	pthread_mutex_unlock(&lock_fd_set);
	
	struct timeval tv;

	while (TRUE) { 
		pthread_mutex_lock(&lock_fd_set);
		memcpy(&rdset, &aus, sizeof(fd_set));
		max_fd = ausmax; 
		pthread_mutex_unlock(&lock_fd_set);
		
		tv.tv_sec = 0;
		tv.tv_usec = 1000;

		int ret = select(max_fd+1, &rdset, NULL, NULL, &tv);
		//Verifico di non aver ricevuto il segnale di terminazione
		pthread_mutex_lock(&mtx_stop);
		if(stop) {
			pthread_mutex_unlock(&mtx_stop);
			break;
		}
		pthread_mutex_unlock(&mtx_stop);
		if(ret == -1) {
			if (errno == EINTR) continue;
			else {
				perror("select");
				exit(errno);
			}
		}
		else {
			for(int i = 0; i<=max_fd; i++) {
				if(FD_ISSET(i, &rdset)) {
					if(i == fd_skt) {
						
						if ((fd_client = accept(fd_skt, NULL, 0)) == -1) {
								perror("accept");
								exit(EXIT_FAILURE);
						}
							
						int free = 1;
						pthread_mutex_lock(&lock_stats);
						if(chattyStats.nonline >= MaxConnections){
							free = 0;
							chattyStats.nerrors++;
						}
						pthread_mutex_unlock(&lock_stats);

						//Non posso accettare la connessione perchè ho troppi utenti online
						if(free == 0){
							printf("Raggiunto numero massimo di connessioni!\n");
							message_hdr_t err;
							memset(&err,0,sizeof(message_hdr_t));
							err.op = OP_FAIL;
							sendHeader(fd_client,&err);
							close(fd_client);
							printf("Chiudo (%d)\n",fd_client);
						}
						else {
							pthread_mutex_lock(&lock_fd_set);
							FD_SET(fd_client, &aus);
							if(fd_client > ausmax ) ausmax = fd_client;	
							pthread_mutex_unlock(&lock_fd_set);
						}
						
					}
					else {
						//Un utente mi ha mandato qualcosa oppure il suo fd è chiuso
						pthread_mutex_lock(&lock_fd_set);
						FD_CLR(i, &aus);
						if( i == ausmax ) ausmax = updatemax(aus, ausmax);
						pthread_mutex_unlock(&lock_fd_set);

						pthread_mutex_lock(&lock_fd_list);
						insert(i);
						pthread_cond_signal(&cond_fd_list);
						pthread_mutex_unlock(&lock_fd_list);
						
					}
				}
			}
			
		}
		
	}
	printf("Listener: ricevuto segnale di terminazione.");
	for(int i = 0; i<=max_fd; i++)
		close(i);
	return (void *)0;
}

static void *worker() {
	message_t rcv;
	message_t snd;

	memset(&snd, 0, sizeof(message_t));
	memset(&snd, 0, sizeof(message_t));
	
	pthread_mutex_t* fdl;

	bool_t op_result = TRUE;
	bool_t breaked = FALSE;
	bool_t disconnect = TRUE;
	int fd = -1;
	
	while (TRUE) {
		if(!breaked) {
			pthread_mutex_lock(&lock_fd_list);
			
			pthread_mutex_lock(&mtx_stop);
			if(stop) {
				pthread_mutex_unlock(&mtx_stop);
				breaked = TRUE;
				break;	
			}
			pthread_mutex_unlock(&mtx_stop);

			while (fd_list_head == NULL) {
				pthread_cond_wait(&cond_fd_list, &lock_fd_list);

				pthread_mutex_lock(&mtx_stop);
				if(stop) {
					pthread_mutex_unlock(&mtx_stop);
					breaked = TRUE;
					break;	
				}
				pthread_mutex_unlock(&mtx_stop);
			}
			if(breaked) {
				pthread_cond_signal(&cond_fd_list);
				pthread_mutex_unlock(&lock_fd_list);
				break;	
			}

			fd = extract();

			printf("\nPrelevato (%d)\n", fd);

			if(fd_list_head != NULL) pthread_cond_signal(&cond_fd_list);
			pthread_mutex_unlock(&lock_fd_list);
		}

		/*Insert_elem restituisce la posizione della lock dell'elemento appena inserito.
		Se l'elemento è già presente vieni restituito l' indice della sua lock*/
		int index  = insert_elem(fd_locks,fd);
		//printf("INDICE (%d) -- FD (%d)\n",index,fd);
		if(index == -1) {
			breaked = TRUE;
			continue;
		}
		else breaked = FALSE;
		
		int letto = 0;

		fdl = extract_at_index(fd_locks, index);

		letto = readMsg_lock(fd, *fdl, &rcv);

		if(letto == -1){
			op_result = FALSE;
			disconnect = TRUE;
		}
		else 
			op_result = TRUE;
		
		// Se ho letto qualcosa controllo quale operazione è stata richiesta e la eseguo
		if(op_result) {
			printf("Operazione richiesta da [%s]\n", rcv.hdr.sender);
		
			switch (rcv.hdr.op) {

				case (REGISTER_OP):
				case (CONNECT_OP):
				case (USRLIST_OP): {
					op_t esito = OP_OK;

					if (rcv.hdr.op == REGISTER_OP)
					{
						printf("---------Eseguo REGISTRAZIONE-------\n");
						if(find_group(rcv.hdr.sender)==-1)
							esito = insert_hash(ht, rcv.hdr.sender, fd);
						else esito = OP_FAIL;
					}

					else if (rcv.hdr.op == CONNECT_OP)
					{
						printf("---------Eseguo CONNESSIONE---------\n");
						esito = hash_connect(ht, rcv.hdr.sender, fd);
						if (esito == OP_OK ) group_connect(rcv.hdr.sender,fd);
					}

					else if (rcv.hdr.op == USRLIST_OP)
					{
						printf("----------Eseguo USRLIST-----------\n");
						esito = find_hash(ht, rcv.hdr.sender);	
					}

					setHeader(&(snd.hdr), esito,"");
					
					if ( esito == OP_OK ) {	
						int connected = MAX_NAME_LENGTH;
						// Recupero la lista dei connessi
						char *aus = connected_users(ht, &connected);

						setData(&(snd.data), rcv.hdr.sender, aus, connected+1);
						
						if( sendRequest_lock(fd, *fdl, &snd) < 0)
							perror("sendRequest_lock_conn");
						
						if (rcv.hdr.op == REGISTER_OP) {
							pthread_mutex_lock(&lock_stats);
							chattyStats.nusers ++;
							chattyStats.nonline ++;
							pthread_mutex_unlock(&lock_stats);

						} 

						if (rcv.hdr.op == CONNECT_OP){
							pthread_mutex_lock(&lock_stats);								
							chattyStats.nonline ++;
							pthread_mutex_unlock(&lock_stats);
							
						}

						if(rcv.hdr.op == CONNECT_OP || rcv.hdr.op == REGISTER_OP)
							insert_fduser(fd,rcv.hdr.sender);
						
						op_result = TRUE;
						free(aus);
					}
					else if (esito == OP_FAIL || esito == OP_NICK_ALREADY || esito == OP_NICK_TOOLONG 
						|| esito == OP_NICK_UNKNOWN) {	
					
						if ( sendHeader_lock(fd,*fdl,&(snd.hdr)) < 0) perror("sendHeader");
					
						printf("operazione non effettuata: Esito (%d)\n", esito);
						pthread_mutex_lock(&lock_stats);
						chattyStats.nerrors ++;
						pthread_mutex_unlock(&lock_stats);

						op_result = FALSE;
						if(rcv.hdr.op == CONNECT_OP) disconnect = FALSE;
					}	
				} break;	

				case (POSTFILE_OP): {

					op_t esito = OP_OK;
					
					printf("-----------Eseguo POSTFILE------------\n");
					printf("Destinatario [%s]\n", rcv.data.hdr.receiver);

					message_data_t file_content;

					//leggo contenuto del file
					readData_lock(fd,*fdl, &(file_content));
					
					if (file_content.hdr.len > (MaxFileSize*1024)) {
						fprintf(stderr,"File troppo grande!\n");
						pthread_mutex_lock(&lock_stats);
						chattyStats.nerrors ++;
						pthread_mutex_unlock(&lock_stats);

						free(file_content.buf);

						setHeader(&(snd.hdr),OP_MSG_TOOLONG,"");
						op_result = FALSE;
						if(sendHeader_lock(fd,*fdl, &(snd.hdr))<0)
							perror("sendheader_post");
						break;
					}
					
					//Costruisco il path
					int filename_len = strlen(rcv.data.buf) + 1;
					int lenght = (strlen(DirName) + 1 + filename_len);
					char* path = calloc(lenght, sizeof(char));
					path = strcat(path,DirName);
					path = strcat(path,"/");
					
					//Cerco l' ultimo '/'
					char* slash = strrchr(rcv.data.buf, '/');
					if(slash) {
						slash++;
						strcat(path,slash);
					}
					else strcat(path, rcv.data.buf);

					FILE* fp;
					//Apro un file e ci scrivo quello che mi è stato inviato
					fp =fopen(path,"w+");
					//valutare se cambiare con fopen
					if(fp == NULL) {
						setHeader(&(snd.hdr),OP_FAIL,"");
						sendHeader_lock(fd,*fdl, &(snd.hdr));
						free(file_content.buf);
						free(path);
						fclose(fp);
						op_result = FALSE;
						break;
					}
					if (fwrite(file_content.buf,1,file_content.hdr.len,fp) < file_content.hdr.len){
						setHeader(&(snd.hdr),OP_FAIL,"");
						sendHeader_lock(fd,*fdl, &(snd.hdr));
						free(file_content.buf);
						free(path);
						fclose(fp);
						op_result = FALSE;
						break;
					}

					fclose(fp);
					free(path);
					free(file_content.buf);

					rcv.hdr.op = FILE_MESSAGE;
					message_t* to_forward;
					to_forward = clone_msg(&rcv);

					//controllo se il ricevente è un gruppo
					if(find_group(rcv.data.hdr.receiver)== 0){

						printf("E' un gruppo...\n");

						//se il sender fa parte del gruppo allora invio il file
						if(isin_group(rcv.data.hdr.receiver,rcv.hdr.sender)==0) {

							int n=0;
							char* nconngroup;
							int* fds_group;
							//Lista dei file descriptor degli utenti del gruppo che sono online
							fds_group = fdonline_group(rcv.data.hdr.receiver);

							for(int k = 0; k < MaxConnections+1; k++) {
								if(fds_group[k]!=0){
									printf("invio a fd: %d\n",fds_group[k]);
									pthread_mutex_t* fd_rec_lock = find(fd_locks, fds_group[k]);
									//NEL MENTRE POTREBBE ESSERE STATO CHIUSO L'FD E LA LOCK ELIMINATA
									if(fd_rec_lock != NULL) {
										if (sendRequest_lock(fds_group[k], *fd_rec_lock ,to_forward) < 0) 
											perror("Sendrequest_lock_group");	
										pthread_mutex_lock(&lock_stats);
										chattyStats.nfiledelivered ++;
										pthread_mutex_unlock(&lock_stats);
									}	
								}	
							}

							free(fds_group);

							//Lista con i nomi degli utenti del gruppo che sono offline
							nconngroup = notconn_group(rcv.data.hdr.receiver,&n);
							int nusers = n / MAX_NAME_LENGTH;
							for(int i=0, p=0; i<nusers;  ++i, p+=(MAX_NAME_LENGTH+1)) {
								printf("salvo il file nella lista di [%s]\n", &nconngroup[p]);
								esito = hash_hangingmessages(ht,*to_forward,&nconngroup[p]);
								pthread_mutex_lock(&lock_stats);
								chattyStats.nfilenotdelivered ++;
								pthread_mutex_unlock(&lock_stats);

							}

							setHeader(&(snd.hdr),OP_OK,"");
							free(nconngroup);
						}
						else { //il sender non fa parte del gruppo
							setHeader(&(snd.hdr),OP_NICK_UNKNOWN,"");
							pthread_mutex_lock(&lock_stats);
							chattyStats.nerrors ++;
							pthread_mutex_unlock(&lock_stats);
							op_result = FALSE;
						}
					}
					else { //Il receiver non è un gruppo
						int fd_rec = 0;
						//controllo se il receiver è online
						fd_rec = is_online(ht,rcv.data.hdr.receiver);
						if(fd_rec == -1) { //è offline metto l'ide del file nei messaggi pendenti
							printf("Utente non online... salvo nella lista dei messaggi pendenti\n");
							esito = hash_hangingmessages(ht, *to_forward, to_forward->data.hdr.receiver);
							printf("Esito operazione %d\n",esito);
							if(esito != OP_OK) {
								setHeader(&(snd.hdr), esito, "");
								pthread_mutex_lock(&lock_stats);
								chattyStats.nerrors ++;
								pthread_mutex_unlock(&lock_stats);
								op_result = FALSE;
							}
							else {
								setHeader(&(snd.hdr), OP_OK, "");
								pthread_mutex_lock(&lock_stats);
								chattyStats.nfilenotdelivered ++;
								pthread_mutex_unlock(&lock_stats);
								op_result = TRUE;
							}
						}
						else { //receiver online
							printf("Utente online... invio identificatore del file su %d\n",fd_rec);
							pthread_mutex_t* f = find(fd_locks,fd_rec);
							if(f == NULL){ //receiver è stato disconnesso
								hash_hangingmessages(ht, *to_forward, to_forward->data.hdr.receiver);
								pthread_mutex_lock(&lock_stats);
								chattyStats.nfilenotdelivered ++;
								pthread_mutex_unlock(&lock_stats);
							}
							else {
								if( sendRequest_lock(fd_rec,*f,to_forward) < 0)
									perror("sendRequest_lock_postfile a dest");

								pthread_mutex_lock(&lock_stats);
								chattyStats.nfilenotdelivered ++;
								pthread_mutex_unlock(&lock_stats);
	
							}
							setHeader(&(snd.hdr), OP_OK, "");
							op_result = TRUE; 
						}	
					}
					//invio il riscontro a chi ha richiesto l'operazione
					if(sendHeader_lock(fd, *fdl, &(snd.hdr))<0)
						perror("sendHeader_lock_postfile a mitt");
					free(to_forward->data.buf);
					free(to_forward);
				} break;
					
				case (UNREGISTER_OP): {
					op_t esito = OP_OK;

					printf("---------Eseguo DEREGISTRAZIONE---------\n");

					esito = hash_delete(ht, rcv.hdr.sender);
					deleteuser_allgroup(rcv.hdr.sender);
					setHeader(&(snd.hdr),esito,"");

					if(esito == OP_OK) {
						pthread_mutex_lock(&lock_stats);
						chattyStats.nusers--;
						chattyStats.nonline--;
						pthread_mutex_unlock(&lock_stats);
					}

					//in ogni caso devo chiudere il file descriptor
					op_result = FALSE;
					//non devo disconnettere l'utente
					disconnect = FALSE; 

					if (sendHeader_lock(fd,*fdl, &(snd.hdr)) < 0)
						perror("sendHeader_lock_unregister");	
	
				} break;

				case (POSTTXT_OP): {	

					printf("--------Eseguo POSTTXT--------\n");
					printf("Destinatario [%s]\n", rcv.data.hdr.receiver);

					//Verifico che il messaggio non sia troppo lungo
					if ( rcv.data.hdr.len > MaxMsgSize ) {
						setHeader(&(snd.hdr), OP_MSG_TOOLONG, "");
						if (sendHeader_lock(fd, *fdl, &(snd.hdr))<0)
							perror("sendheader_lock_posttxt");

						pthread_mutex_lock(&lock_stats);
						chattyStats.nerrors ++;
						pthread_mutex_unlock(&lock_stats);
						
						op_result = FALSE;
						
						break;
					}

					message_t* to_forward;
					rcv.hdr.op = TXT_MESSAGE;
					to_forward = clone_msg(&rcv);

					//se il receiver è un gruppo
					if(find_group(rcv.data.hdr.receiver)== 0){
						printf("E' un gruppo...\n");
						//se il sender è ne gruppo
						if(isin_group(rcv.data.hdr.receiver,rcv.hdr.sender)==0) {

							int n=0;
							char* nconngroup;
							int* fds_group;
							fds_group = fdonline_group(rcv.data.hdr.receiver);

							for(int k = 0; k < MaxConnections+1; k++) {
								if(fds_group[k]!=0){
									
									printf("invio a fd: %d\n",fds_group[k]);
									pthread_mutex_t* fd_rec_lock = find(fd_locks, fds_group[k]);
									//NEL MENTRE POTREBBE ESSERE STATO CHIUSO L'FD E LA LOCK ELIMINATA
									if(fd_rec_lock != NULL) {
										if (sendRequest_lock(fds_group[k], *fd_rec_lock ,to_forward) < 0) 
												perror("Sendrequest_lock_group");	
										pthread_mutex_lock(&lock_stats);
										chattyStats.ndelivered ++;
										pthread_mutex_unlock(&lock_stats);
									}	
								}	
							}

							free(fds_group);

							nconngroup = notconn_group(rcv.data.hdr.receiver,&n);
							int nusers = n / MAX_NAME_LENGTH;
							for(int i=0, p=0; i<nusers;  ++i, p+=(MAX_NAME_LENGTH+1)) {
								printf("salvo il msg nella lista di [%s]\n", &nconngroup[p]);
								hash_hangingmessages(ht,*to_forward, &nconngroup[p]);
								pthread_mutex_lock(&lock_stats);
								chattyStats.nnotdelivered ++;
								pthread_mutex_unlock(&lock_stats);
							}

							setHeader(&(snd.hdr),OP_OK,"");
							free(nconngroup);

						} else {
							setHeader(&(snd.hdr),OP_NICK_UNKNOWN,"");
							pthread_mutex_lock(&lock_stats);
							chattyStats.nerrors ++;
							pthread_mutex_unlock(&lock_stats);
							op_result = FALSE;
						}	
					}
					else {
						int rcv_fd = is_online(ht, rcv.data.hdr.receiver);
						switch(rcv_fd) {
							case 0: {
								//Il ricevente non esiste
								printf("Destinatario sconosciuto!\n");
								setHeader(&(snd.hdr), OP_FAIL, "");
								op_result = FALSE;

							} break;
							case -1: { //ricevente offline
								printf("Il ricevente non è online\n");
								hash_hangingmessages(ht, *to_forward, to_forward->data.hdr.receiver);
								setHeader(&(snd.hdr), OP_OK, "");
								
								//Aggiorno le statistiche
								pthread_mutex_lock(&lock_stats);
								chattyStats.nnotdelivered ++;
								pthread_mutex_unlock(&lock_stats);

								op_result = TRUE;	

							} break;

							default: { 
								//Il ricevente esiste ed è online
								printf("[%s] è online, invio messaggio in fd %d\n",  rcv.data.hdr.receiver,rcv_fd);
								//Inoltro subito il messaggio
								pthread_mutex_t* fdrcl = find(fd_locks, rcv_fd);
								if(fdrcl == NULL){
									hash_hangingmessages(ht, *to_forward, to_forward->data.hdr.receiver);
									setHeader(&(snd.hdr), OP_OK, "");
									
									//Aggiorno le statistiche
									pthread_mutex_lock(&lock_stats);
									chattyStats.nnotdelivered ++;
									pthread_mutex_unlock(&lock_stats);
								}
								else {
									if(sendRequest_lock(rcv_fd, *fdrcl, to_forward)<0)
										perror("sendRequest_lock_posttxt");
									
									setHeader(&(snd.hdr), OP_OK, "");
									//aggiorno le statistiche
									pthread_mutex_lock(&lock_stats);
									chattyStats.ndelivered ++;
									pthread_mutex_unlock(&lock_stats);
								}
									op_result = TRUE;
							} break;
						}
					}
					free(to_forward -> data.buf);
					free(to_forward);
					if(sendHeader_lock(fd, *fdl, &(snd.hdr))<0)
						perror("sendHeader_lock_posttxt");	
				}
				break;

				case (POSTTXTALL_OP): {

					op_t esito = OP_OK;
					printf("\n");
					printf("---------Eseguo POSTTXTALL---------\n");

					//Verifico che il messaggio non sia troppo grande
					if ( rcv.data.hdr.len > MaxMsgSize ) {
						fprintf(stderr,"messaggio troppo grande!\n POSTTXTALL non eseguita\n");
						setHeader(&(snd.hdr), OP_MSG_TOOLONG, "");
						
						if (sendHeader_lock(fd,*fdl, &(snd.hdr))<0)
							perror("sendHeader_lock_postall");
						
						pthread_mutex_lock(&lock_stats);
						chattyStats.nerrors ++;
						pthread_mutex_unlock(&lock_stats);
						
						op_result = FALSE;
						break;
					}

					message_t* to_forward;
					rcv.hdr.op = TXT_MESSAGE;
					to_forward = clone_msg(&rcv);

					printf("Invio il messaggio agli utenti online...\n");
					int n_active_fd;
					int * active_fd;
					//restituisce array di file descriptor di utenti online
					active_fd = array_elem(fd_locks, &n_active_fd);

					for(int k = 0; k < n_active_fd; k++) {
						if( active_fd[k] != fd ) {
							pthread_mutex_t* fd_rec_lock = find(fd_locks, active_fd[k]);
							//NEL MENTRE POTREBBE ESSERE STATO CHIUSO L'FD E LA LOCK ELIMINATA
							if(fd_rec_lock != NULL) {
								if (sendRequest_lock(active_fd[k], *fd_rec_lock ,to_forward) < 0) {
									perror("Sendrequest_lock_postall");
								}
								pthread_mutex_lock(&lock_stats);
								chattyStats.ndelivered ++;
								pthread_mutex_unlock(&lock_stats);
							}
						}
					}

					free(active_fd);

					printf("Salvataggio nella lista dei messaggi degli utenti che non sono online...\n");
					esito = hash_messages_toall(ht,*to_forward,to_forward->hdr.sender);
					if (esito == OP_OK) {
						//aggiorno statistiche
						pthread_mutex_lock(&lock_stats);
						int m = chattyStats.nusers - chattyStats.nonline;
						chattyStats.nnotdelivered = chattyStats.nnotdelivered + m;
						pthread_mutex_unlock(&lock_stats);

						op_result = TRUE;
						setHeader(&(snd.hdr), OP_OK, "");
					} else {
						op_result = FALSE;
						setHeader(&(snd.hdr), OP_FAIL, "");
					}
					
					free(to_forward->data.buf);
					free(to_forward);

					if (sendHeader_lock(fd,*fdl, &(snd.hdr))<0)
						perror("sendH_lock_postall");
				}
				break;
				case (GETFILE_OP): {
				
					printf("---------Eseguo GETFILE---------\n");

					//Costruisco il path
					int filename_len = strlen(rcv.data.buf) + 1;
					int lenght = (strlen(DirName) + 1 + filename_len);
					char* path = calloc(lenght, sizeof(char));
					path = strcat(path,DirName);
					path = strcat(path,"/");
					
					//Cerco l' ultimo '/'
					char* slash = strrchr(rcv.data.buf, '/');
					if(slash) {
						slash++;
						strcat(path,slash);
					}
					else strcat(path, rcv.data.buf);

					FILE* fp;
					//buffer che conterrà il contenuto del file
					char* filecont = malloc(((MaxFileSize*1024)+1)*sizeof(char));
					fp = fopen(path,"w+");
					fseek(fp,0,SEEK_SET);
					int letti = fread(filecont,sizeof(filecont),1,fp);

					if ( letti < 0 ) { 
						perror("readn_getfile");
						fclose(fp);
						free(path);
						free(filecont);
						op_result = FALSE;
						setHeader(&(snd.hdr), OP_FAIL, ""); 
						sendHeader_lock(fd, *fdl, &(snd.hdr));
					} 
					else {
						fclose(fp);
						setHeader(&(snd.hdr), OP_OK, "");
						setData(&(snd.data), rcv.hdr.sender, filecont, letti);
						if( sendRequest_lock(fd, *fdl, &snd) < 0)
							perror("sendRequest_lock_getfile");
						//aggiorno statistiche
						pthread_mutex_lock(&lock_stats);
						chattyStats.nfiledelivered++;
						chattyStats.nfilenotdelivered--;
						pthread_mutex_unlock(&lock_stats);
						free(filecont);
						free(path);
						op_result = TRUE;
					}
				} break;

				case (GETPREVMSGS_OP): {
					
					printf("---------Eseguo GETPREVMSGS---------\n");

					unsigned long num;
					message_t **prevmsg;
					prevmsg = extract_hangingmessages(ht,rcv.hdr.sender,&num);

					printf("Numero di messaggi da mandare: %lu\n",num);
					
					if (num < 0) {
						setHeader(&(snd.hdr),OP_FAIL,"");
						
						if (sendHeader_lock(fd,*fdl, &(snd.hdr))<0)
							perror("sendHeader_lock_getmsg");
						
						pthread_mutex_lock(&lock_stats);
						chattyStats.nerrors++;
						pthread_mutex_unlock(&lock_stats);
						
						op_result = FALSE;	

						break;
					}

					setHeader(&(snd.hdr), OP_OK, "");
					setData(&(snd.data), rcv.hdr.sender, (char*)&num, sizeof(size_t));
					//invio il numero di messaggi
					if (sendRequest_lock(fd, *fdl, &snd) < 0)
						perror("sendRequest_lock_getmsg");
					//invio tutti i messaggi
					int n_file = 0, n_text = 0;
					for (int j = 0; j < num; j++) {
						printf("Invio messaggio #%d\n", j);
						if(prevmsg[j]->hdr.op == FILE_MESSAGE) 
							n_file++;
						if(prevmsg[j]->hdr.op == TXT_MESSAGE)
							n_text++;
						if (sendRequest_lock(fd, *fdl, prevmsg[j]) < 0) {
							op_result = FALSE;
							perror("get_prev_lock");
						}
						free(prevmsg[j]->data.buf);
						free(prevmsg[j]);
					}
					free(prevmsg);
					pthread_mutex_lock(&lock_stats);
					chattyStats.nfiledelivered+=n_file;
					chattyStats.nfilenotdelivered-=n_file;
					chattyStats.ndelivered+=n_text;
					chattyStats.nnotdelivered-=n_text;
					pthread_mutex_unlock(&lock_stats);
					op_result = TRUE;
				}
				break;

				//operazione sui gruppi
				case (CREATEGROUP_OP):
				case (ADDGROUP_OP): 
				case (DELGROUP_OP):{

					op_t esito = OP_OK;
					if(rcv.hdr.op == CREATEGROUP_OP){ //CREA UN GRUPPO
						printf("---------Eseguo CREATEGROUP--------\n");
						printf("Si vuole aggiungere il gruppo %s'\n",rcv.data.hdr.receiver);
						esito = add_group(rcv.data.hdr.receiver,rcv.hdr.sender,fd);
					}
					if(rcv.hdr.op == ADDGROUP_OP){ //AGGIUNGE SENDER AL GRUPPO
						printf("---------Eseguo ADDGROUP-------\n");
						printf("l'utente vuole aggiungersi al gruppo [%s]\n",rcv.data.hdr.receiver);
						esito = adduser_group(rcv.data.hdr.receiver,rcv.hdr.sender,fd);
					}
					if(rcv.hdr.op == DELGROUP_OP){//RIMUOVE SENDER DAL GRUPPO
						printf("l'utente vuole eliminarsi dal gruppo [%s]\n",rcv.data.hdr.receiver);
						esito = deleteuser_group(rcv.data.hdr.receiver,rcv.hdr.sender);
					}
					if(esito == OP_OK){
						printf("Operazione %d effettuata!\n",rcv.hdr.op);
						op_result = TRUE;
					}
					else {
						pthread_mutex_lock(&lock_stats);
						chattyStats.nerrors++;
						pthread_mutex_unlock(&lock_stats);
					}
					print_group();
					setHeader(&(snd.hdr),esito,"");
					sendHeader_lock(fd,*fdl,&(snd.hdr));
			
				} break;

				default:{
					fprintf(stderr,"ERRORE: Operazione non riconosciuta\n");
					op_result = FALSE;
				} break;
			}	
		}

		free(rcv.data.buf);
		//Se l'operazione è fallita chiudo e disconnetto se disconnect = TRUE
		if(!op_result) {
			printf("--------Eseguo DISCONNECT-------\n");
			//Se devo disconnettere metto offline l'utente nelle strutture dati
			if(disconnect){
				char* user = malloc(sizeof(char)*(MAX_NAME_LENGTH +1));
				//ottengo il nome dell'utente da disconnettere
				extract_fduser(fd,&user); 
				printf("disconnetto.. %s\n",user);
				//disconnetto dalla tabella hash
				op_t opt = hash_disconnect(ht,user);
				//disconnetto dai gruppi
				group_disconnect(user);
				free(user);
			
				if(opt == OP_FAIL)
					printf("Esito disconnessione negativo\n");
				else {
					printf("Disconnessione eseguita\n");
					pthread_mutex_lock(&lock_stats);
					chattyStats.nonline --;
					pthread_mutex_unlock(&lock_stats);
				}
			}
			delete_elem(fd_locks, fd);
			close(fd);
			printf("Chiuso (%d)\n",fd);
		}
		//Se l'operazione si è conclusa con successo rimetto l'fd in ascolto
		else if(op_result) {
			pthread_mutex_lock(&lock_fd_set);
			FD_SET(fd,&aus);
			if(fd > ausmax) ausmax = fd;
			pthread_mutex_unlock(&lock_fd_set);
		}	
	}//end while
	return (void *)0;
}
static void * handler() {
	
	sigset_t set;
	if (sigemptyset(&set) != 0) { 
		perror("Sigemptyset"); 
		exit(EXIT_FAILURE);
	}
	if ( sigaddset(&set, SIGINT) != 0 || sigaddset(&set, SIGQUIT) != 0
		|| sigaddset(&set, SIGTERM) != 0 || sigaddset(&set, SIGPIPE) != 0
		|| sigaddset(&set, SIGUSR1) != 0 || sigaddset(&set, SIGUSR2) != 0 ) { 
	
		perror("Sigaddset"); exit(EXIT_FAILURE);		
	} 

	pthread_sigmask(SIG_SETMASK, &set, NULL);
	
	int sig = 0;

	while(TRUE) {

		sigwait(&set, &sig);	
		
		if(sig == SIGTERM || sig == SIGQUIT || sig == SIGINT) {
			pthread_mutex_lock(&mtx_stop);
			stop = TRUE;
			pthread_mutex_unlock(&mtx_stop);

			pthread_mutex_lock(&lock_fd_list);
			pthread_cond_broadcast(&cond_fd_list);
			pthread_mutex_unlock(&lock_fd_list);

			return NULL;
		}

		else if(sig == SIGUSR1) {

			FILE* stat = fopen(StatFileName, "a+");
			printStats(stat);
			fclose(stat);
		}
	}
}