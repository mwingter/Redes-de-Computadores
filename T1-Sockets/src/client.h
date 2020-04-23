#ifndef CLIENT_H

	#define CLIENT_H

	
	//####### Funcoes ########

	void str_overwrite_stdout();

	void str_trim_lf(char* arr, int length);

	void catch_ctrl_c_and_exit();

	void recv_msg_handler();

	void send_msg_handler();


#endif