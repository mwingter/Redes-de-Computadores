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

#define MAX_CLIENTS 100 	//Numero maximo de clientes conectados
#define BUFFER_SZ 4096 		//Tamanho limite para cada mensagem
#define BUFFER_AUX 1000000 	//Auxiliar para mensagens maiores que o limite de 4096
#define NAME_LEN 50		//Tamanho maximo para nome do cliente

volatile sig_atomic_t flag = 0;
int sockfd = 0;
char name[NAME_LEN];

bool onChannel = false;


/*
 * str_overwrite_stdout
 *
 * Funcao que atualiza a tela com um novo "> " em uma nova linha
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
 * sigintHandler
 *
 * Funcao que verifica se o comando "Ctrl + C" foi pressionado e ignora-o.
 * 
 * @param 	pre		Substring
 *			str		String a ser verificada
 *               
 */
void sigintHandler(int sig_num){ 
    /* Reseta o handler para pegar o SIGINT na próxima vez. */
    signal(SIGINT, sigintHandler); 
    printf("\n # Não é possível sair pressionando Ctrl+C #\n"); 
    fflush(stdout); 
}


/*
 * catch_ctrl_c_and_exit
 *
 * Funcao que atualiza a flag para desconexão do cliente. Se flag = 1, o cliente é desconectado do servidor.
 *                
 */
void catch_ctrl_c_and_exit(){
	flag = 1;
}

/*
 * recv_msg_handler
 *
 * Funcao que gerencia o recebimento de mensagens.
 *               
 */
void recv_msg_handler(){
	char message[BUFFER_SZ] = {};

	while(1){
		int receive = recv(sockfd, message, BUFFER_SZ, 0);

		if(receive > 0){
			if(startsWith("/saidasala ", message)){
				onChannel = false;
			}
			else{
				printf("%s ", message);
			}
			str_overwrite_stdout();
			
			
		}
		else if(receive == 0){
			break;
		}
		bzero(message, BUFFER_SZ);
	}
}

/*
 * send_msg_handler
 *
 * Funcao que gerencia o enviamento de mensagens.
 *               
 */
void send_msg_handler(){
	char buffer[BUFFER_SZ+1] = {};
	char buffer_aux[BUFFER_AUX] = {};
	char message[BUFFER_SZ + NAME_LEN];
	int isCtrlD = 0;

	while(1){
		str_overwrite_stdout();
		isCtrlD = 0;

		while(fgets(buffer_aux, BUFFER_AUX, stdin) != NULL && isCtrlD == 0){
			str_trim_lf(buffer_aux, BUFFER_AUX);
			if(strlen(buffer_aux) == 0){
				str_overwrite_stdout();
			}
			else{
				isCtrlD = 1;
				break;
			}

		}

		if(strcmp(buffer_aux, "/quit") == 0 || isCtrlD == 0){
			flag = 1;
			break;
		}

		else if(startsWith("/nickname ", buffer_aux)){
			if(strlen(buffer_aux) >  NAME_LEN+10){
				printf("Nickname grande demais.\n");
			}
			else{
				send(sockfd, buffer_aux, strlen(buffer_aux), 0);
				strncpy(name, &buffer_aux[10], 50);
			}
		}
		else if(startsWith("/join ", buffer_aux)){
			if(strlen(buffer_aux) >  205){
				printf("Nome de canal grande demais.\n");
			}
			else{
				if(onChannel == false){
					send(sockfd, buffer_aux, strlen(buffer_aux), 0);
					onChannel = true;
				}
				else{
					printf("ERRO: Você já está em um canal. Para entrar em outro canal, você precisa sair do atual primeiro, digitando: /leavechannel\n\n");
				}
			}
		}
		else if(strcmp(buffer_aux, "/ping") == 0){
			send(sockfd, buffer_aux, strlen(buffer_aux), 0);
		}
		else if(strcmp(buffer_aux, "/leavechannel") == 0){
			if (onChannel == true){
				send(sockfd, buffer_aux, strlen(buffer_aux), 0);
				onChannel = false;	
			}
			else{
				printf("ERRO: Você não está em nenhum canal.\n\n");
			}
		}
		else if(startsWith("/kick ", buffer_aux) || startsWith("/mute ", buffer_aux) || startsWith("/unmute ", buffer_aux) || startsWith("/whois ", buffer_aux)){
			send(sockfd, buffer_aux, strlen(buffer_aux), 0);
		}
		else{
			if(strlen(buffer_aux) <  BUFFER_SZ){ //se a mensagem tiver até o tamanho máximo, envia normal
				if(strcmp(name, "-1") == 0){
					sprintf(message, "%s\n", buffer_aux);
				}
				else{
					sprintf(message, "%s: %s\n", name, buffer_aux);
				}
				send(sockfd, message, strlen(message), 0);
			}
			else{ //se a mensagem for maior que o tamanho máximo, será dividida em várias mensagens e enviadas em partes
				int count = strlen(buffer_aux)/BUFFER_SZ;
				if(strlen(buffer_aux) % BUFFER_SZ != 0){
					count++; //quantidade de mensagens
				}

				for (int i = 0; i < count; i++){
					for (int j = 0; j < BUFFER_SZ; j++){
						buffer[j] = buffer_aux[BUFFER_SZ*i+j];
					}
					buffer[BUFFER_SZ] = '\0';
					
					if(strcmp(name, "-1") == 0){
						sprintf(message, "%s\n", buffer);
					}
					else{
						sprintf(message, "%s: %s\n", name, buffer);
					}

					send(sockfd, message, strlen(message), 0);

					bzero(buffer, BUFFER_SZ);
					bzero(message, BUFFER_SZ + NAME_LEN);

				}

			}
		}

		bzero(buffer, BUFFER_SZ);
		bzero(buffer_aux, BUFFER_SZ);
		bzero(message, BUFFER_SZ + NAME_LEN);
	}
	catch_ctrl_c_and_exit(2);
}


int main(int argc, char const *argv[])
{
	signal(SIGINT, sigintHandler);

	printf("\n- Para conectar ao servidor, digite: /connect\n- Para escolher um nickname, digite: /nickname <nickname_desejado>\n- Para sair, digite: /quit ou pressione Ctrl + D\n");

	char enterChat[100] = "/connect";
	strcpy(name, "-1");
	while(1){
		
		if(fgets(enterChat, 100, stdin) == NULL || strcmp(enterChat, "/quit\n") == 0){
			printf("\nVolte sempre!\n");
			return 0;
		}
		str_trim_lf(enterChat, 100);

		if(strcmp(enterChat, "/connect\0") == 0){
			break;
		}
		else if(startsWith("/nickname ", enterChat)){
			if(strlen(enterChat) >=  NAME_LEN+10 || strlen(enterChat) < 12){
				printf("Nickname grande ou pequeno demais.\n");
			}
			else{
				strncpy(name, &enterChat[10], NAME_LEN-1); //salvando o nickname na variavel name
				printf("Novo nickname: %s\n", name);
				str_trim_lf(name, strlen(name));
			}
		}

		else{
			printf("\nComando inválido.\n- Para conectar ao servidor, digite: /connect\n- Para escolher um nickname, digite: /nickname <nickname_desejado>\n- Para sair do chat, digite: /quit ou pressione Ctrl + D\n");
		}
		//bzero(enterChat, BUFFER_SZ);
	}


	if(argc != 2){
		printf("Como usar: %s <numero-da-porta>\n", argv[0]);
		return EXIT_FAILURE;
	}
	

	char *ip = "127.0.0.1";
	int port = atoi(argv[1]);

	//signal(SIGINT, catch_ctrl_c_and_exit);


	struct sockaddr_in server_addr;
	//socket settings
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip);
	server_addr.sin_port = htons(port);

	//connect to the server
	int err = connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));

	if(err == -1){
		printf("ERRO: Falha ao conectar ao servidor.\n");
		return EXIT_FAILURE;
	}

	//send the nickname
	//strcpy(name, "-1");
	send(sockfd, name, NAME_LEN, 0);

	printf("\n=== OLÁ! BEM-VINDO AO CHAT [PORTA %d] ===\n", port);
	printf("_____________________________________________________________________________________________\n");
	printf("  INSTRUÇÕES:\n - Para mandar uma mensagem, basta digitar ao lado do simbolo '>' abaixo e teclar Enter para enviar\n - Para escolher um nickname, digite: /nickname <Nickname_Desejado>\n - Para sair do chat, digite: /quit ou pressione Ctrl + D\n - Digite /ping para receber do servidor um retorno 'pong' assim que este receber a mensagem.\n");
	printf("_____________________________________________________________________________________________\n\n");

	signal(SIGINT, sigintHandler); 


	pthread_t send_msg_thread;
	if(pthread_create(&send_msg_thread, NULL, (void*)send_msg_handler, NULL) != 0){
		printf("ERRO: Falha ao enviar a mensagem.\n");
		return EXIT_FAILURE;
	}

	pthread_t recv_msg_thread;
	if(pthread_create(&recv_msg_thread, NULL, (void*)recv_msg_handler, NULL) != 0){
		printf("ERRO: Falha ao receber a mensagem.\n");
		return EXIT_FAILURE;
	}

	while(1){
		if(flag){
			printf("\nVolte sempre!\n");
			break;
		}
	}

	close(sockfd); //desconecta o cliente

	return 0;
}