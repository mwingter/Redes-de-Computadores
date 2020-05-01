/* TRABALHO 1 - REDES

	Nome: Michelle Wingter da Silva
	nUSP: 10783243
*/

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>

#include "server.h"

#define MAX_CLIENTS 100
#define BUFFER_SZ 1000

static _Atomic unsigned int cli_count = 0;
static int uid = 10;


client_t *clients[MAX_CLIENTS];

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;


/*
 * str_overwrite_stdout
 *
 * Funcao que atualiza a tela com um novo "> "
 *              
 */
void str_overwrite_stdout(){
	printf("\r%s", "> ");
	fflush(stdout);
}

/*
 * str_trim_lf
 *
 * Funcao que substitui o ultimo caracter de uma string, se este for '\n', por '\0'
 * 
 * @param 	arr			String a ser modificada
 *			length		Tamanho da string
 *               
 */
void str_trim_lf(char* arr, int length){
	for(int i = 0; i < length; i++){
		if(arr[i] == '\n'){
			arr[i] = '\0';
			break;
		}
	}
}

/*
 * queue_add
 *
 * Funcao que 
 * 
 * @param 	cl 	Ponteiro para a estrutura do cliente
 *               
 */
void queue_add(client_t *cl){
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i < MAX_CLIENTS; i++){
		if(!clients[i]){
			clients[i] = cl;
			break;
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

/*
 * queue_add
 *
 * Funcao que 
 * 
 * @param 	cl 	Ponteiro para a estrutura do cliente
 *               
 */
void queue_remove(int uid){
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i<MAX_CLIENTS; i++){
		if(clients[i]){
			if(clients[i]->uid == uid){
				clients[i] = NULL;
				break;
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

void print_ip_addr(struct sockaddr_in addr){
	printf("%d.%d.%d.%d", addr.sin_addr.s_addr & 0xff, 
						(addr.sin_addr.s_addr & 0xff00) >> 8,
						(addr.sin_addr.s_addr & 0xff0000) >> 16,
						(addr.sin_addr.s_addr & 0xff000000) >> 24
						);
}

void send_message(char *s, int uid){
	pthread_mutex_lock(&clients_mutex);

	for (int i = 0; i < MAX_CLIENTS; i++){
		if(clients[i]){
			if(clients[i]->uid != uid){
				if(write(clients[i]->sockfd, s, strlen(s)) < 0){
					printf("ERROR: write to descriptor failed\n");
					break;
				}
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

void *handle_client(void *arg){
	char buffer[BUFFER_SZ];
	char name[NAME_LEN];
	int leave_flag = 1;
	cli_count++;

	client_t *cli = (client_t*)arg;

	//name
	do{
		if(recv(cli->sockfd, name, NAME_LEN, 0) <= 0 || strlen(name) < 2 || strlen(name) >= NAME_LEN - 1){
			printf("Digite seu nome corretamente.\n");
		}
		else
			leave_flag=0;

	}while(leave_flag==1);
	
		//utilizando strcpy_s para proteger contra buffer overflow
		strncpy(cli->name, name, 32);
		sprintf(buffer, "%s entrou no chat.\n", cli->name);
		printf("%s", buffer);
		send_message(buffer, cli->uid);

	bzero(buffer, BUFFER_SZ);

	while(1){
		if(leave_flag){
			break;
			
		}

		int receive = recv(cli->sockfd, buffer, BUFFER_SZ, 0);

		if(receive > 0){
			if(strlen(buffer) > 0){
				send_message(buffer, cli->uid);
				str_trim_lf(buffer, strlen(buffer));

				printf("%s\n", buffer);
			}
		}
		else if(receive == 0 || strcmp(buffer, "exit") == 0){
			sprintf(buffer, "%s saiu do chat.\n", cli->name);
			printf("%s", buffer);
			send_message(buffer, cli->uid);
			leave_flag = 1;
		}
		else{
			printf("ERROR: -1\n");
			leave_flag = 1;
		}
	
		bzero(buffer, BUFFER_SZ);

	}

	close(cli->sockfd);
	queue_remove(cli->uid);
	free(cli);
	cli_count--;
	pthread_detach(pthread_self());

	return NULL;
}

int main(int argc, char const *argv[])
{
	if(argc != 2){
		printf("Como usar: %s <numero-da-porta>\n", argv[0]);
		return EXIT_FAILURE;
	}

	char *ip = "127.0.0.1";
	int port = atoi(argv[1]);

	int option = 1;
	int listenfd = 0, connfd = 0;
	struct sockaddr_in serv_addr;
	struct sockaddr_in cli_addr;
	pthread_t tid;

	//socket settings
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(ip);
	serv_addr.sin_port = htons(port);

	//signals
	signal(SIGPIPE, SIG_IGN);

	if(setsockopt(listenfd, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), (char*)&option, sizeof(option)) < 0){
		printf("ERROR: setsockopt\n");
		return EXIT_FAILURE;
	}

	//bind
	if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
		printf("ERROR: bind\n");
		return EXIT_FAILURE;
	}

	//listen
	if(listen(listenfd, 10) < 0){
		printf("ERROR: listen\n");
		return EXIT_FAILURE;
	}

	printf("=== NOVO CHAT [PORTA %s] CRIADO ===\n", argv[1]);

	while(1){
		socklen_t clilen = sizeof(cli_addr);
		connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);
		
		//check for MAX-CLIENTS
		if((cli_count + 1) == MAX_CLIENTS){
			printf("Maximo de clientes conectados. Coneccao Rejeitada. ");
			print_ip_addr(cli_addr);
			close(connfd);
			continue;
		}

		//client settings
		client_t *cli = (client_t *)malloc(sizeof(client_t));
		cli->adress = cli_addr;
		cli->sockfd = connfd;
		cli->uid = uid++;

		//add client to queue
		queue_add(cli);
		pthread_create(&tid, NULL, &handle_client, (void*)cli);

		//reduce CPU usage
		sleep(1);

	}



	return 0;
}