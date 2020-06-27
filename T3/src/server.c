/* TRABALHO 3 - REDES
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
int muted_client[1001]; //vetor que sinaliza se um cliente está mutado ou não. 1 = mutado, 0 = não mutado.

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;


/*
 * str_overwrite_stdout
 *
 * Funcao que atualiza a tela com um novo "> " em uma nova linha             
 */
void str_overwrite_stdout(){
	printf("\r%s", "> ");
	fflush(stdout);
}

/*
 * str_trim_lf
 *
 * Funcao que substitui o ultimo caracter de uma string se este for '\n', por '\0'
 * 
 * @arr			String a ser modificada
 * @length		Tamanho da string             
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
 * Funcao que verifica se uma string 'str' se inicia com uma outra string 'pre'
 * 
 * @pre		Substring
 * @str		String a ser verificada             
 */
bool startsWith(const char *pre, const char *str){
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);

    return lenstr < lenpre ? false : memcmp(pre, str, lenpre) == 0;
}

/*
 * queue_add
 *
 * Funcao que coloca os clientes em uma fila
 * 
 * @cl 	Ponteiro para a estrutura do cliente            
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
 * @uid 	Uid do cliente a ser removido da fila            
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
 * Funcao que printa um endereço de ip de um cliente
 * 
 * @addr 	Endereço de Ip           
 */
void print_ip_addr(struct sockaddr_in addr){
	printf("%d.%d.%d.%d", addr.sin_addr.s_addr & 0xff, 
						(addr.sin_addr.s_addr & 0xff00) >> 8,
						(addr.sin_addr.s_addr & 0xff0000) >> 16,
						(addr.sin_addr.s_addr & 0xff000000) >> 24
						);
}

/*
 * string_ip_addr
 *
 * Funcao que salva na string ip o endereço de ip de um cliente
 * 
 * @addr 	Endereço de ip de um cliente
 * @ip 		String onde será salvo o ip            
 */
void string_ip_addr(struct sockaddr_in addr, char* ip){
	sprintf(ip, "\n---> O IP do cliente é: %d.%d.%d.%d\n", addr.sin_addr.s_addr & 0xff, 
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
 * @s 		String da mensagem a ser enviada
 * @uid 	Id do cliente             
 */
void send_message(char *s, int uid){
	if(muted_client[uid] == 1 && strcmp(client_channel[uid], "-1") != 0){ //verifica se o cliente que esta enviando a msg está mutado
		sprintf(s, "\nERRO: Você está mutado no canal <%s> - não pode mandar mensagens neste canal.\n", client_channel[uid]);
		respond_message(s, uid);
		return;
	}
	pthread_mutex_lock(&clients_mutex);

	for (int i = 0; i < MAX_CLIENTS; i++){
		if(clients[i]){
			if(clients[i]->uid != uid){
				if(strcmp(client_channel[uid],client_channel[clients[i]->uid]) == 0){ //verifica se os clientes estão no mesmo canal
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
 * @s 		String da mensagem a ser enviada
 * @uid 	uid do cliente          
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

/*
 * reset_channel
 *
 * Funcao que reseta os valores da struct de um canal
 * 
 * @index 	Índice do canal no vetor 'channels'            
 */
void reset_channel(int index){
	strcpy(channels[index]->ch_name, "-1");
	channels[index]->admin_id = -1;
	strcpy(channels[index]->admin_name, "-1");
	channels[index]->num_users = 0;
	for (int i = 0; i < MAX_CLIENTS; i++){
		muted_client[channels[index]->clients[i]] = 0; //desmuta os clientes do canal resetado
		channels[index]->clients[i] = -1;
	}
}
/*
void close_channel(int index){
	char msg[BUFFER_SZ];

	sprintf(msg, "\nCanal <%s> FECHADO pelo admin <%s>\n> Você foi desconectado do canal.\n", channels[index]->ch_name, channels[index]->admin_name);
	printf("Canal <%s> FECHADO pelo admin <%s>\n", channels[index]->ch_name, channels[index]->admin_name);
	respond_message(msg, channels[index]->admin_id);
	respond_message("/kick aaa", channels[index]->admin_id);


	if (channels[index]->num_users == 0){ //se o canal não tem clientes para remover, finaliza
		printf("Todos usuários foram desconectados do canal <%s>\n", channels[index]->ch_name);
	}

	for (int i = 0; i < MAX_CLIENTS; i++){
		if(channels[index]->num_users == 0){
			printf("Todos usuários foram desconectados do canal <%s>\n", channels[index]->ch_name);
			break;
		}
		if(channels[index]->clients[i] != -1){
			client_channel[channels[index]->clients[i]] = "-1";
			channels[index]->num_users --;
			respond_message(msg, channels[index]->clients[i]);
			respond_message("/kick aaa", channels[index]->clients[i]);
			muted_client[channels[index]->clients[i]] = 0; //desmuta o cliente kickado
			channels[index]->clients[i] = -1;
		}
	}
	client_channel[channels[index]->admin_id] = "-1"; //removendo o admin do canal


	//send_message(msg, channels[index]->admin_id);
	//channels[index] = NULL; //removendo o canal
	reset_channel(index);
}
*/


/*
 * close_channel
 *
 * Funcao que fecha um canal aberto. Remove o admin e todos clientes e chama a função que reseta o canal.
 * 
 * @ch_index 	Índice do canal no vetor 'channels'              
 */
void close_channel(int ch_index){
	char msg[BUFFER_SZ];
	sprintf(msg, "\nCanal <%s> FECHADO pelo admin <%s>\n> Você foi desconectado do canal.\n", channels[ch_index]->ch_name, channels[ch_index]->admin_name);
	printf("Canal <%s> FECHADO pelo admin <%s>\n", channels[ch_index]->ch_name, channels[ch_index]->admin_name);
	
	respond_message(msg, channels[ch_index]->admin_id);
	respond_message("/saidasala ", channels[ch_index]->admin_id);

	for (int i = 0; i < MAX_CLIENTS; i++){
		if(channels[ch_index]->num_users == 0){ //se o canal não tem clientes para remover, finaliza
			printf("Todos usuários foram desconectados do canal <%s>\n", channels[ch_index]->ch_name);
			break;
		}
		if(channels[ch_index]->clients[i] != -1){
			char name_cli[NAME_LEN] = {};
			for (int j = 0; j < MAX_CLIENTS; j++){
				if (channels[ch_index]->clients[i] == clients[j]->uid){
					strncpy(name_cli, clients[j]->name, NAME_LEN);
					break;
				}
			}
			kick_user(channels[ch_index]->clients[i], name_cli, ch_index);
		}
	}
	client_channel[channels[ch_index]->admin_id] = "-1"; //removendo o admin do canal

	reset_channel(ch_index);
}

/*
 * kick_user
 *
 * Funcao que desconecta um cliente de um canal
 * 
 * @cli_id		Uid do cliente que será desconectado
 * @cli_name	Nome do cliente que será desconectado
 * @ch_ind 		Índice do canal no vetor 'channels'             
 */
void kick_user(int cli_id, char* cli_name, int ch_ind){
	char msg[BUFFER_SZ];
	sprintf(msg, "### Você foi desconectado do canal <%s>\n", client_channel[cli_id]);
	respond_message(msg, cli_id);

	sprintf(msg, "/saidasala ");
	respond_message(msg, cli_id);
	
	muted_client[cli_id] = 0; //desmutando o cliente caso esteja mutado
	sprintf(msg, "### O cliente <%s> foi desconectado do canal <%s>\n", cli_name, client_channel[cli_id]);
	printf("%s\n", msg);
	send_message(msg, cli_id);

	

	client_channel[cli_id]= "-1";
	channels[ch_ind]->num_users--;
	for (int i = 0; i < MAX_CLIENTS; i++){
		if(channels[ch_ind]->clients[i] == cli_id){
			channels[ch_ind]->clients[i] = -1;
			break;
		}
	}	
}

/*
 * mute_user
 *
 * Funcao que muta um cliente de um canal para que este não possa enviar mensagens neste canal
 * 
 * @cli_id		Uid do cliente que será mutado
 * @cli_name	Nome do cliente que será mutado
 * @ch_ind 		Índice do canal no vetor 'channels'              
 */
void mute_user(int cli_id, char* cli_name, int ch_ind){
	char msg[BUFFER_SZ];
	if(muted_client[cli_id] == 1){ //se o cliente ja estiver mutado
		sprintf(msg, "\nERRO: O cliente <%s> já está mutado.\n", client_channel[cli_id]);
		respond_message(msg, cli_id);
		return;		
	}
	sprintf(msg, "\n### Você foi mutado no canal <%s>\n", client_channel[cli_id]);
	respond_message(msg, cli_id);
	sprintf(msg, "\n### O cliente <%s> foi mutado no canal <%s>\n", cli_name, client_channel[cli_id]);
	send_message(msg, cli_id);
	printf("%s\n", msg);

	muted_client[cli_id] = 1;
}

/*
 * unmute_user
 *
 * Funcao que desmuta um cliente mutado em um canal para que este possa voltar a enviar mensagens neste canal
 * 
 * @cli_id		Uid do cliente que será mutado
 * @cli_name	Nome do cliente que será mutado
 * @ch_ind 		Índice do canal no vetor 'channels'              
 */
void unmute_user(int cli_id, char* cli_name, int ch_ind){
	char msg[BUFFER_SZ];
	if(muted_client[cli_id] == 0){ //se o cliente ja estiver desmutado
		sprintf(msg, "\nERRO: O cliente <%s> já está desmutado.\n", client_channel[cli_id]);
		respond_message(msg, cli_id);
		return;		
	}

	muted_client[cli_id] = 0;

	sprintf(msg, "\n### Você foi desmutado no canal <%s>\n", client_channel[cli_id]);
	respond_message(msg, cli_id);
	sprintf(msg, "\n### O cliente <%s> foi desmutado no canal <%s>\n", cli_name, client_channel[cli_id]);
	send_message(msg, cli_id);
	printf("%s\n", msg);
}

/*
 * handle_client
 *
 * Thread que cuida de cada cliente              
 */
void *handle_client(void *arg){
	char buffer[BUFFER_SZ];
	char message[BUFFER_SZ];
	char name[NAME_LEN];
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
					strncpy(name, &buffer[10], NAME_LEN);
					sprintf(buffer, "--- O cliente <%s> trocou o nickname para: <%s>\n", cli->name, name);
					send_message(buffer, cli->uid);

					strncpy(cli->name, name, NAME_LEN);
					sprintf(buffer, "--- Você trocou seu nickname para: <%s>\n", cli->name);
					respond_message(buffer, cli->uid);

					if(strcmp(client_channel[cli->uid],"-1") != 0){ //se o cliente é admin de um canal, atualiza o nome do admin deste canal
						for (int i = 0; i < MAX_CHANNELS; i++){
							if(!(channels[i])){
								break;
							}
							if(cli->uid == channels[i]->admin_id){ //procurando canal administrado pelo cliente
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
					char channel_name[CHANNEL_NAME_LEN];
					str_trim_lf(buffer, strlen(buffer));
					strncpy(channel_name, &buffer[6], 200);
					printf("$$$ O cliente <%s> esta no canal <%s> e quer entrar no canal <%s>\n", cli->name , client_channel[cli->uid], channel_name);

					if(strcmp(client_channel[cli->uid],"-1") != 0){ //se o cliente já estiver em um canal
						sprintf(buffer, "ERRO: Você já está no canal <%s>. Para entrar em outro canal, você precisa sair do atual primeiro, digitando: /leavechannel\n\n", client_channel[cli->uid]);
						respond_message(buffer, cli->uid);
					}

					else{
						printf("O CLIENTE NAO ESTA EM UM CANAL\n");
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
						if(ch_existe == false){ // se o canal não tiver já sido criado antes
							for (int i = 0; i < MAX_CHANNELS; i++){
								if(!(channels[i]) || strcmp(channels[i]->ch_name, "-1") == 0){
									channel_c *ch = (channel_c *)malloc(sizeof(channel_c));
									strncpy(ch->ch_name, channel_name, CHANNEL_NAME_LEN);
									ch->admin_id = cli->uid;
									strncpy(ch->admin_name, cli->name, NAME_LEN);
									ch->num_users = 0;
									for (int i = 0; i < MAX_CLIENTS; ++i){
										ch->clients[i] = -1;
									}

									channels[i] = ch;
									ch_index = i;
									ch_existe = true; //canal criado!
									printf("\n---> NOVO CANAL criado: <%s>, admin: <%s> <---\n\n", channels[i]->ch_name, channels[i]->admin_name);
									break;
								}
							}
						}
						if(ch_existe == false){ //se o canal não foi criado, significa que já tem o máximo de canais permitido
							sprintf(buffer, "ERRO. Não é possível criar mais canais.\n");
							printf("%s\n", buffer);
						}
						else{
							client_channel[cli->uid] = channel_name; //colocando o cliente no canal

							sprintf(buffer, "### O cliente <%s> entrou no canal <%s>\n", cli->name, client_channel[cli->uid]);
							printf("%s", buffer);
							send_message(buffer, cli->uid);
							printf("----- Admin do Canal <%s>: %s, ID: %d ------\n", channels[ch_index]->ch_name, channels[ch_index]->admin_name, channels[ch_index]->admin_id);
							sprintf(buffer, "\n=========================================\nVocê entrou no canal <%s>\n=========================================\n----- Admin do Canal <%s>: %s, ID: %d ------\n[ - Para sair do canal, digite: /leavechannel ]\n\n", channel_name, channel_name, channels[ch_index]->admin_name, channels[ch_index]->admin_id);
							respond_message(buffer, cli->uid);
							if (strcmp(cli->name, channels[ch_index]->admin_name) != 0){ //se o cliente que entrou no canal não for o admin
								channels[ch_index]->num_users++;
								for (int i = 0; i < MAX_CLIENTS; ++i){
									if(channels[ch_index]->clients[i] == -1){
										channels[ch_index]->clients[i] = cli->uid;
										break;
									}
								}
							}
							else{ //o cliente que entrou no canal é o admin																																																																																		  
								sprintf(buffer, "**** COMANDOS DO ADMIN: ****\n 	/kick <Nickname> - Fecha a conexão do usuário especificado;\n 	/mute <Nickname> - Muta o usuário para que não possa ouvir mensagens neste canal;\n 	/unmute <Nickname> - Retira o mute de um usuário;\n 	/whois <Nickname> - Retorna o endereço IP do usuário apenas para o admin.\n> ****************************\n\n");
								respond_message(buffer, cli->uid);																		
							}
						}
					}
					
				}
				else if(strcmp(buffer, "/leavechannel") == 0){ //Sai do canal, caso esteja em um
					if(strcmp(client_channel[cli->uid], "-1") == 0){ //se o cliente não estiver em nenhum canal
						sprintf(buffer, "ERRO: Comando inválido. Você não está em nenhum canal.\n");
						respond_message(buffer, cli->uid);
					}
					else{ //o cliente está em um canal
						char ch_leave[CHANNEL_NAME_LEN+1]; //nome do canal 
						int ind = -1;
						strncpy(ch_leave, client_channel[cli->uid], CHANNEL_NAME_LEN);

						for (int i = 0; i < MAX_CHANNELS; i++){ //procurando pelo canal de nome ch_leave
							if(strcmp(channels[i]->ch_name, ch_leave) == 0){
								ind = i; //canal encontrado
								break;
							}
						}
						if (ind == -1){
							printf("!!!!! FALHA !!!! canal não encontrado\n");
						}

						if(strcmp(cli->name, channels[ind]->admin_name) == 0){ //verifica se quem está saindo do canal é o admin
							close_channel(ind); //se for o admin saindo, o canal é fechado
						}
						else{ //se não for o admin saindo
							kick_user(cli->uid, cli->name, ind);
						}

					}
				}

				//COMANDOS DO ADMIN => kick, mute, unmute, whois
				else if(startsWith("/kick ", buffer)){ //Fecha a conexão de um usuário especificado
					if(strcmp(client_channel[cli->uid], "-1") == 0){ //se o cliente não estiver em nenhum canal
						sprintf(buffer, "ERRO: Comando inválido. Você precisa estar em um canal e ser admin para isso.\n");
						respond_message(buffer, cli->uid);
					}
					else{ //o cliente está em um canal
						char user_to_kick[NAME_LEN]; //usuário que será kickado do canal
						str_trim_lf(buffer, strlen(buffer));
						strncpy(user_to_kick, &buffer[6], NAME_LEN);

						char ch_kick[CHANNEL_NAME_LEN+1]; //nome do canal 
						int ind = -1;
						strncpy(ch_kick, client_channel[cli->uid], CHANNEL_NAME_LEN);

						for (int i = 0; i < MAX_CHANNELS; i++){ //procurando pelo canal de nome ch_kick
							if(strcmp(channels[i]->ch_name, ch_kick) == 0){
								ind = i; //canal encontrado
								break;
							}
						}
						if (ind == -1){
							printf("!!!!! FALHA !!!! canal não encontrado\n");
						}

						if(strcmp(cli->name, channels[ind]->admin_name) != 0){ //verificando se o cliente que enviou o comando não é admin do canal
							sprintf(buffer, "ERRO: Você não tem permissão para este comando. Apenas admins possuem autorização para isto.\n");
							respond_message(buffer, cli->uid);
						} 
						else{
							if(strcmp(user_to_kick, cli->name) == 0){ //se o próprio adm estiver se kickando, a sala é fechada e todos usuarios serão kickados
								close_channel(ind);
								continue;
							}

							int user_found = 0;
							//procurando o id do usuario que será kickado
							for (int i = 0; i < MAX_CLIENTS; i++){
								if(clients[i]){
									if(strcmp(clients[i]->name, user_to_kick) == 0){ //se encontrar o usuario com este nick
										kick_user(clients[i]->uid, clients[i]->name, ind);
										user_found = 1;
										break;
									}
								}
							}
							if(user_found == 0){
								sprintf(buffer, "ERRO: Nenhum cliente de nickname <%s> no canal.\n", user_to_kick);
								respond_message(buffer, cli->uid);
							}			
						}
					}

				}

				else if(startsWith("/mute ", buffer)){ //Faz com que um usuário não possa enviar mensagens neste canal
					if(strcmp(client_channel[cli->uid], "-1") == 0){ //se o cliente não estiver em nenhum canal
						sprintf(buffer, "ERRO: Comando inválido. Você precisa estar em um canal e ser admin para isso.\n");
						respond_message(buffer, cli->uid);
					}
					else{ //o cliente está em um canal
						char user_to_mute[NAME_LEN]; //usuário que será kickado do canal
						str_trim_lf(buffer, strlen(buffer));
						strncpy(user_to_mute, &buffer[6], NAME_LEN);
						if(strcmp(user_to_mute, cli->name) == 0){ //se o próprio adm tentar se mutar
							sprintf(buffer, "\nERRO: Não é permitido mutar/desmutar o admin do canal.\n\n");
							respond_message(buffer, cli->uid);
						}
						else{
							
							char ch_mute[CHANNEL_NAME_LEN+1]; //nome do canal 
							int ind = -1;
							strncpy(ch_mute, client_channel[cli->uid], CHANNEL_NAME_LEN);

							for (int i = 0; i < MAX_CHANNELS; i++){ //procurando pelo canal de nome ch_mute
								if(strcmp(channels[i]->ch_name, ch_mute) == 0){
									ind = i; //canal encontrado
									break;
								}
							}
							if (ind == -1){
								printf("!!!!! FALHA !!!! canal não encontrado\n");
							}
							if(strcmp(cli->name, channels[ind]->admin_name) != 0){ //verificando se o cliente que enviou o comando é admin do canal
								sprintf(buffer, "ERRO: Você não tem permissão para este comando. Apenas admins possuem autorização para isto.\n");
								respond_message(buffer, cli->uid);
							}
							else{ //o cliente que mandou o comando é o admin
								int user_found = 0;
								for (int i = 0; i < MAX_CLIENTS; i++){ //procurando o cliente que será mutado
									if(clients[i]){
										if(strcmp(clients[i]->name, user_to_mute) == 0){ //se encontrar o usuario com este nick
											mute_user(clients[i]->uid, clients[i]->name, ind);
											user_found = 1;
											break;
										}
									}
								}
								if(user_found == 0){
									sprintf(buffer, "ERRO: Nenhum cliente de nickname <%s> no canal.\n", user_to_mute);
									respond_message(buffer, cli->uid);
								}
							}

						}


					}
				}
				else if(startsWith("/unmute ", buffer)){ //Faz com que um usuário não possa enviar mensagens neste canal
					if(strcmp(client_channel[cli->uid], "-1") == 0){ //se o cliente não estiver em nenhum canal
						sprintf(buffer, "ERRO: Comando inválido. Você precisa estar em um canal e ser admin para isso.\n");
						respond_message(buffer, cli->uid);
					}
					else{ //o cliente está em um canal
						char user_to_unmute[NAME_LEN]; //usuário que será kickado do canal
						str_trim_lf(buffer, strlen(buffer));
						strncpy(user_to_unmute, &buffer[8], NAME_LEN);
						if(strcmp(user_to_unmute, cli->name) == 0){ //se o próprio adm tentar se desmutar
							sprintf(buffer, "\nERRO: Não é permitido mutar/desmutar o admin do canal.\n\n");
							respond_message(buffer, cli->uid);
						}
						else{
							char ch_unmute[CHANNEL_NAME_LEN+1]; //nome do canal 
							int ind = -1;
							strncpy(ch_unmute, client_channel[cli->uid], CHANNEL_NAME_LEN);

							for (int i = 0; i < MAX_CHANNELS; i++){ //procurando pelo canal de nome ch_unmute
								if(strcmp(channels[i]->ch_name, ch_unmute) == 0){
									ind = i; //canal encontrado
									break;
								}
							}
							if (ind == -1){
								printf("!!!!! FALHA !!!! canal não encontrado\n");
							}
							if(strcmp(cli->name, channels[ind]->admin_name) != 0){ //verificando se o cliente que enviou o comando não é admin do canal
								sprintf(buffer, "ERRO: Você não tem permissão para este comando. Apenas admins possuem autorização para isto.\n");
								respond_message(buffer, cli->uid);
							}
							else{ //o cliente é o admin
								int user_found = 0;
								for (int i = 0; i < MAX_CLIENTS; i++){ //procurando o cliente que será desmutado
									if(clients[i]){
										if(strcmp(clients[i]->name, user_to_unmute) == 0){ //se encontrar o usuario com este nick
											unmute_user(clients[i]->uid, clients[i]->name, ind);
											user_found = 1;
											break;
										}
									}
								}
								if(user_found == 0){
									sprintf(buffer, "ERRO: Nenhum cliente de nickname <%s> no canal.\n", user_to_unmute);
									respond_message(buffer, cli->uid);
								}
							}

						}


					}
				}
				else if(startsWith("/whois ", buffer)){ //Retorna o endereço IP do usuário apenas para o administrador
					if(strcmp(client_channel[cli->uid], "-1") == 0){ //verifica se o cliente não está em um canal
						sprintf(buffer, "ERRO: Comando inválido. Você precisa estar em um canal e ser admin para isso.\n");
						respond_message(buffer, cli->uid);
					}
					else{ //o cliente está em um canal
						char user_ip_name[NAME_LEN]; //usuário que será kickado do canal
						str_trim_lf(buffer, strlen(buffer));
						strncpy(user_ip_name, &buffer[7], NAME_LEN);

						char ch_ip[CHANNEL_NAME_LEN+1]; //nome do canal 
						int ind = -1;
						strncpy(ch_ip, client_channel[cli->uid], CHANNEL_NAME_LEN);

						for (int i = 0; i < MAX_CHANNELS; i++){ //procurando pelo canal de nome ch_unmute
							if(strcmp(channels[i]->ch_name, ch_ip) == 0){
								ind = i; //canal encontrado
								break;
							}
						}
						if (ind == -1){
							printf("!!!!! FALHA !!!! canal não encontrado\n");
						}

						if(strcmp(cli->name, channels[ind]->admin_name) != 0){ //verificando se o cliente que enviou o comando não é admin do canal
								sprintf(buffer, "ERRO: Você não tem permissão para este comando. Apenas admins possuem autorização para isto.\n");
								respond_message(buffer, cli->uid);
						}
						else{ //o cliente é o admin
							int user_found = 0;
							for (int i = 0; i < MAX_CLIENTS; i++){ //procurando o cliente que será desmutado
								if(clients[i]){
									if(strcmp(clients[i]->name, user_ip_name) == 0){ //se encontrar o usuario com este nick
										char ip[100];
										string_ip_addr(clients[i]->adress, ip);
										respond_message(ip, cli->uid);
										user_found = 1;
										break;
									}
								}
							}
							if(user_found == 0){
								sprintf(buffer, "ERRO: Nenhum cliente de nickname <%s> no canal.\n", user_ip_name);
								respond_message(buffer, cli->uid);
							}
						}

						
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
			if(strcmp(client_channel[cli->uid], "-1") != 0){ //se o cliente estiver em um canal e der comando de sair do chat
				int ch_index = -1;
				for (int i = 0; i < MAX_CHANNELS; ++i){ //procurando o canal que o cliente está
					if(strcmp(channels[i]->ch_name, client_channel[cli->uid]) == 0){
						ch_index = i;
						break;
					}
				}
				if(strcmp(channels[ch_index]->admin_name, cli->name) == 0){ //se o cliente for o adm do canal
					close_channel(ch_index);
				}
				else{
					kick_user(cli->uid, cli->name, ch_index);
				}
			}
			
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
		muted_client[i] = 0;
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