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

#define MAX_CLIENTS 100 	//Numero maximo de clientes conectados
#define BUFFER_SZ 4096 		//Este eh o limite de tamanho para cada mensagem
#define BUFFER_AUX 1000000 	//Auxiliar para mensagens maiores que o limite de 4096
#define NAME_LEN 32 		//Tamanho maximo para nome do cliente

volatile sig_atomic_t flag = 0;
int sockfd = 0;
char name[NAME_LEN];

void str_overwrite_stdout(){
	printf("\r%s", "> ");
	fflush(stdout);
}

void str_trim_lf(char* arr, int length){
	for(int i = 0; i < length; i++){
		if(arr[i] == '\n'){
			arr[i] = '\0';
			break;
		}
	}
}

void catch_ctrl_c_and_exit(){
	flag = 1;
}

void recv_msg_handler(){
	char message[BUFFER_SZ] = {};

	while(1){
		int receive = recv(sockfd, message, BUFFER_SZ, 0);

		if(receive > 0){
			printf("%s ", message);
			str_overwrite_stdout();
			
			
		}
		else if(receive == 0){
			break;
		}
		bzero(message, BUFFER_SZ);
	}
}

void send_msg_handler(){
	char buffer[BUFFER_SZ+1] = {};
	char buffer_aux[BUFFER_AUX] = {};
	char message[BUFFER_SZ + NAME_LEN];

	while(1){
		str_overwrite_stdout();

		fgets(buffer_aux, BUFFER_AUX, stdin);

		str_trim_lf(buffer_aux, BUFFER_AUX);

		if(strcmp(buffer_aux, "exit") == 0){
			break;
		}
		else{
			if(strlen(buffer_aux) <  BUFFER_SZ){
				sprintf(message, "%s: %s\n", name, buffer_aux);
				send(sockfd, message, strlen(message), 0);
			}
			else{
				int count = strlen(buffer_aux)/BUFFER_SZ;
				if(strlen(buffer_aux) % BUFFER_SZ != 0){
					count++; //quantidade de mensagens
				}

				for (int i = 0; i < count; i++){
					for (int j = 0; j < BUFFER_SZ; j++){
						buffer[j] = buffer_aux[BUFFER_SZ*i+j];
					}
					buffer[BUFFER_SZ] = '\0';
					
					sprintf(message, "%s: %s\n", name, buffer);

					send(sockfd, message, strlen(message), 0);

					bzero(buffer, BUFFER_SZ);
					bzero(message, BUFFER_SZ + NAME_LEN);

				}

			}
		}

		bzero(buffer, BUFFER_SZ);
		bzero(message, BUFFER_SZ + NAME_LEN);
	}
	catch_ctrl_c_and_exit(2);
}


int main(int argc, char const *argv[])
{
	if(argc != 2){
		printf("Como usar: %s <numero-da-porta>\n", argv[0]);
		return EXIT_FAILURE;
	}

	char *ip = "127.0.0.1";
	int port = atoi(argv[1]);

	signal(SIGINT, catch_ctrl_c_and_exit);

	printf("Digite seu nome: ");
	fgets(name, NAME_LEN, stdin);
	str_trim_lf(name, strlen(name));

	if(strlen(name) > NAME_LEN - 1 || strlen(name) < 2){
		printf("Digite seu nome corretamente.\n");
		return EXIT_FAILURE;
	}

	struct sockaddr_in server_addr;
	//socket settings
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip);
	server_addr.sin_port = htons(port);

	//connect to the server
	int err = connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));

	if(err == -1){
		printf("ERROR: connect\n");
		return EXIT_FAILURE;
	}

	//send the name
	send(sockfd, name, NAME_LEN, 0);

	printf("=== OLA, %s. BEM-VINDO AO CHAT [PORTA %s] ===\n", name, argv[1]);

	pthread_t send_msg_thread;
	if(pthread_create(&send_msg_thread, NULL, (void*)send_msg_handler, NULL) != 0){
		printf("ERROR: pthread\n");
		return EXIT_FAILURE;
	}

	pthread_t recv_msg_thread;
	if(pthread_create(&recv_msg_thread, NULL, (void*)recv_msg_handler, NULL) != 0){
		printf("ERROR: pthread\n");
		return EXIT_FAILURE;
	}

	while(1){
		if(flag){
			printf("\nVolte sempre!\n");
			break;
		}
	}

	close(sockfd);

	return 0;
}