#ifndef SERVER_H
	
	#define SERVER_H

	#define NAME_LEN 50
	#define CHANNEL_NAME_LEN 201

	//####### Estrutura cliente ########
	typedef struct{
		struct sockaddr_in adress;
		int sockfd;
		int uid;
		char name[NAME_LEN];
	} client_t;

	//####### Estrutura canal ########
	typedef struct{
		char ch_name[CHANNEL_NAME_LEN];
		int admin_id;
		char admin_name[NAME_LEN];
		int num_users;
		int clients[100];
	} channel_c;


	//####### Funções ########

	void str_overwrite_stdout();

	void str_trim_lf(char* arr, int length);

	bool startsWith(const char *pre, const char *str);

	void queue_add(client_t *cl);

	void queue_remove(int uid);

	void print_ip_addr(struct sockaddr_in addr);

	void string_ip_addr(struct sockaddr_in addr, char* ip);

	void send_message(char *s, int uid);

	void respond_message(char *s, int uid);

	void reset_channel(int index);

	void close_channel(int ch_index);

	void kick_user(int cli_id, char* cli_name, int ch_ind);

	void mute_user(int cli_id, char* cli_name, int ch_ind);

	void unmute_user(int cli_id, char* cli_name, int ch_ind);

	void *handle_client(void *arg);


#endif