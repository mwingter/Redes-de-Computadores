/* TRABALHO 2 - REDES
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
#include <stdbool.h>

#include "server.h"

#define MAX_CLIENTS 100 //Máximo de clientes conectados
#define MAX_CHANNELS 100 //Máximo de canais criados
#define BUFFER_SZ 100000 //Tamanho máximo de mensagem que o servidor pode receber
#define BUFFER_AUX 4096 //Tamanho máximo para cada mensagem recebida

static _Atomic unsigned int cli_count = 0;
static int uid = 0;

client_t *clients[MAX_CLIENTS];

char* client_channel[1001]; //vetor que guarda o nome de qual canal cada cliente está conectado. O indice do vetor é o uid do cliente. Se o cliente não estiver em nenhum canal, o valor será "-1"
channel_c *channels[MAX_CHANNELS]; //vetor que guarda os canais abertos
int channel_count = 0;

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
 * startsWith
 *
 * Funcao que verifica se uma string str se inicia com uma outra string pre
 * 
 * @param 	pre		Substring
 *			str		String a ser verificada
 *               
 */
bool startsWith(const char *pre, const char *str){
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);

    return lenstr < lenpre ? false : memcmp(pre, str, lenpre) == 0;
}

/*
 * queue_add
 *
 * Funcao que coloca os clientes em uma fila, para adicionar um a um no servidor
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
 * queue_remove
 *
 * Funcao que remove o cliente da fila
 * 
 * @param 	uid
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

/*
 * print_ip_addr
 *
 * Funcao que 
 * 
 * @param 	addr 	
 *               
 */
void print_ip_addr(struct sockaddr_in addr){
	printf("%d.%d.%d.%d", addr.sin_addr.s_addr & 0xff, 
						(addr.sin_addr.s_addr & 0xff00) >> 8,
						(addr.sin_addr.s_addr & 0xff0000) >> 16,
						(addr.sin_addr.s_addr & 0xff000000) >> 24
						);
}

/*
 * send_message
 *
 * Funcao que envia mensagem aos clientes conectados no servidor
 * 
 * @param 	s 		String da mensagem a ser enviada
 			uid 	Id do cliente
 *               
 */
void send_message(char *s, int uid){
	pthread_mutex_lock(&clients_mutex);
	//printf("\n====Enviando mensagemmm\n");

	for (int i = 0; i < MAX_CLIENTS; i++){
		if(clients[i]){
			if(clients[i]->uid != uid){
				if(strcmp(client_channel[uid],client_channel[clients[i]->uid]) == 0){ //verifica se os clientes estão no mesmo canal
					//printf("==Agora envia a mensagem!\n\n");
					if(write(clients[i]->sockfd, s, strlen(s)) < 0){
						printf("ERRO: Mensagem não enviada\n");
						break;
					}
				}
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

/*
 * respond_message
 *
 * Funcao que envia mensagem a um cliente de dado uid
 * 
 * @param 	s 		String da mensagem a ser enviada
 			uid 	uid do cliente
 *               
 */
void respond_message(char *s, int uid){
	pthread_mutex_lock(&clients_mutex);

	for (int i = 0; i < MAX_CLIENTS; i++){
		if(clients[i]){
			if(clients[i]->uid == uid){
				if(write(clients[i]->sockfd, s, strlen(s)) < 0){
					printf("ERRO: Mensagem não enviada\n");
					break;
				}
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

void close_channel(int index){



}


/*
 * send_message
 *
 * Thread que cuida de cada cliente
 * 
 * @param 	arg
 *               
 */
void *handle_client(void *arg){
	char buffer[BUFFER_SZ];
	char message[BUFFER_SZ];
	char name[NAME_LEN];
	char channel_name[CHANNEL_NAME_LEN];
	int leave_flag = 0;
	cli_count++;

	client_t *cli = (client_t*)arg;


	if(recv(cli->sockfd, name, NAME_LEN, 0) <= 0 || strlen(name) < 2 || strlen(name) >= NAME_LEN - 1){
		sprintf(buffer, "%s saiu do chat pois o nickname estava fora dos padrões.\n", cli->name);
		printf("%s", buffer);
		send_message(buffer, cli->uid);
		leave_flag = 1;
	}
	else{
		
		if(strcmp(name, "-1") == 0){
			sprintf(cli->name, "Client-%d", cli->uid);
		}
		else{
			strncpy(cli->name, name, 50); //utilizando strncpy para proteger contra buffer overflow
		}
		sprintf(buffer, "%s entrou no chat.\n", cli->name);
		printf("%s", buffer);
		send_message(buffer, cli->uid);
	}

	bzero(buffer, BUFFER_SZ);

	while(1){
		if(leave_flag){
			break;
		}

		int receive = recv(cli->sockfd, buffer, BUFFER_SZ, 0);

		if(receive > 0){
			if(strlen(buffer) > 0){
				if(strcmp(buffer, "/ping") == 0){
					sprintf(buffer, "pong\n");
					//printf("%s", buffer);
					respond_message(buffer, cli->uid);
				}
				else if(startsWith("/nickname ", buffer)){
					strncpy(cli->name, &buffer[10], NAME_LEN);
					strncpy(name, &buffer[10], NAME_LEN);
					sprintf(buffer, "Você trocou seu nickname para: %s\n", cli->name);
					respond_message(buffer, cli->uid);
					if(strcmp(client_channel[cli->uid],"-1") != 0){ //se o cliente é admin de um canal, atualiza o nome do admin deste canal
						for (int i = 0; i < MAX_CHANNELS; ++i){
							if(strcmp(client_channel[cli->uid],channels[i]->ch_name) == 0){ //procurando o canal com o nome do canal administrado pelo cliente
								strncpy(channels[i]->admin_name, cli->name, NAME_LEN);
								sprintf(buffer, "Atualização: ----- Admin do Canal <%s>: %s, ID: %d ------\n", channels[i]->ch_name, cli->name, cli->uid);
								respond_message(buffer, cli->uid);
								send_message(buffer, cli->uid);
								break;
							}
						}
					}
				}
				else if(startsWith("/join ", buffer)){
					if(strcmp(client_channel[cli->uid],"-1") != 0){ //se o cliente já estiver em um canal, e estiver tentando entrar em outro
						for (int i = 0; i < MAX_CHANNELS; ++i){
							if(strcmp(client_channel[cli->uid],channels[i]->ch_name) == 0){
								if(strcmp(cli->name,channels[i]->admin_name) == 0){ //se o cliente for o admin do canal em que está, o canal será fechado
									printf("\n----FECHANDO CANAL---\n");
									close_channel(i);
									break;
								}

							}
						}


						//sprintf(buffer, "ERRO. Não é possível criar mais canais.\n");
					}
					str_trim_lf(buffer, strlen(buffer));
					strncpy(channel_name, &buffer[6], 200);
					
					client_channel[cli->uid] = channel_name;
					

					//buscando se o canal existe, se não existir, cria um novo canal e coloca o primeiro cliente que entrou como admin
					bool ch_existe = false;
					int ch_index = -1;
					for (int i = 0; i < MAX_CHANNELS; i++){
						if((channels[i])){ //se na posição i há um canal
							if(strcmp(channels[i]->ch_name, channel_name) == 0){ // se o canal ja existir
								ch_existe = true;
								ch_index = i;
								break;
							}
						}
					}
					if(ch_existe == false){ // se o canal não tiver já sido criado
						for (int i = 0; i < MAX_CHANNELS; i++){
							if(!(channels[i])){
								channel_c *ch = (channel_c *)malloc(sizeof(channel_c));
								strcpy(ch->ch_name, channel_name);
								ch->admin_id = cli->uid;
								strcpy(ch->admin_name, cli->name);
								ch->users = 1;
								channels[i] = ch;
								ch_index = i;
								ch_existe = true;
								break;
							}
						}
					}
					if(ch_existe == false){ //se o canal não foi criado, significa que já tem o máximo de canais permitido
						sprintf(buffer, "ERRO. Não é possível criar mais canais.\n");
						printf("%s\n", buffer);
					}
					else{
						printf("O cliente %s entrou no canal <%s>\n", cli->name, client_channel[cli->uid]);
						printf("----- Admin do Canal <%s>: %s, ID: %d ------\n", channel_name, channels[ch_index]->admin_name, channels[ch_index]->admin_id);
						sprintf(buffer, "\n=========================================\nVocê entrou no canal <%s>\n=========================================\n----- Admin do Canal <%s>: %s, ID: %d ------\n\n", channel_name, channel_name, channels[ch_index]->admin_name, channels[ch_index]->admin_id);
						respond_message(buffer, cli->uid);
					}

				}

				else if(strlen(buffer) < BUFFER_AUX){
					if(strcmp(name, "-1") == 0){
						sprintf(message, "%s: %s", cli->name, buffer);
						send_message(message, cli->uid);
						str_trim_lf(buffer, strlen(buffer));
						str_trim_lf(message, strlen(message));
						printf("%s\n", message);

					}
					else{
						send_message(buffer, cli->uid);
						str_trim_lf(buffer, strlen(buffer));
						printf("%s\n", buffer);
					}
				}
				else{

					// Caso alguem mande uma mensagem com o tamanho maior que o permitido, ele sera desconectado do servidor
					// Para testar use o commando no terminal "nc localhost 1234" coloque nome, e depois envie uma mensagem maior que 4096
					sprintf(buffer, "%s saiu do chat pois mandou msg maior que o maximo permitido.\n", cli->name);
					printf("%s", buffer);
					send_message(buffer, cli->uid);
					close(cli->sockfd);
					leave_flag = 1;
				}
			}
		}
		else if(receive == 0 || strcmp(buffer, "/quit") == 0){
			sprintf(buffer, "%s saiu do chat.\n", cli->name);
			printf("%s", buffer);
			send_message(buffer, cli->uid);
			leave_flag = 1;
		}

		else{
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

	for (int i = 0; i < 1000; ++i)
	{
		client_channel[i] = "-1";
	}



	// Comandos necessarios para bindar na porta recebira pelo argv[1] esperando uma conexao TCP
	char *ip = "0.0.0.0"; //bindando em todas as interfaces de rede
	int port = atoi(argv[1]);

	int option = 1;
	int listenfd = 0, connfd = 0;
	struct sockaddr_in serv_addr;
	struct sockaddr_in cli_addr;
	pthread_t tid;

	// Configurando socket
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(ip);
	serv_addr.sin_port = htons(port);

	//signals
	signal(SIGPIPE, SIG_IGN);

	// Abrindo socket
	if(setsockopt(listenfd, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), (char*)&option, sizeof(option)) < 0){
		printf("ERRO: Não foi possivel abrir o socket.\n");
		return EXIT_FAILURE;
	}

	//bind
	if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
		printf("ERRO: Porta está sendo usada por outro processo, ou há um problema com o IP definido.\n");
		return EXIT_FAILURE;
	}

	//listen
	if(listen(listenfd, 10) < 0){
		printf("ERRO: Não foi possivel colocar em modo listening.\n");
		return EXIT_FAILURE;
	}

	printf("=== NOVO CHAT [PORTA %s] CRIADO ===\n", argv[1]);

	// Esperando conexao
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

		//Configurando o cliente
		client_t *cli = (client_t *)malloc(sizeof(client_t));
		cli->adress = cli_addr;
		cli->sockfd = connfd;
		cli->uid = uid++;

		//Inicia chat com o client
		queue_add(cli);
		pthread_create(&tid, NULL, &handle_client, (void*)cli);

	}



	return 0;
}