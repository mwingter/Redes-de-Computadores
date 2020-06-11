#ifndef SERVER_H
	
	#define SERVER_H

	#define NAME_LEN 50

	//####### Estrutura cliente ########
	typedef struct{
		struct sockaddr_in adress;
		int sockfd;
		int uid;
		char name[NAME_LEN];
	} client_t;


	//####### Funcoes ########

	void str_overwrite_stdout();

	void str_trim_lf(char* arr, int length);

	bool startsWith(const char *pre, const char *str);

	void queue_add(client_t *cl);

	void queue_remove(int uid);

	void print_ip_addr(struct sockaddr_in addr);

	void send_message(char *s, int uid);

	void respond_message(char *s, int uid);

	void *handle_client(void *arg);


#endif