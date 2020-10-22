#ifndef LIN_UTILS_H
#define LIN_UTILS_H

#define SLLIN_LDISC    25 // sllin line discipline number
#define NUM_LIN_IDS    64 // number of different LIN IDs

int rdy_to_send; // LIN frame caught and ready to be sent
char out[20]; // LIN frame buffer

typedef struct{
	char *device_path;
	char *module_path;
	char *if_name;
	int if_index;
	int baud_rate;
} LIN_params;

#define LIN_params_initializer (LIN_params){NULL, NULL, NULL, 0, 0}

int get_num_LIN_IDs();
void free_LIN_params(LIN_params*);
void print_LIN_params(LIN_params*);
int sllin_interface_up(LIN_params*);
int sllin_config_up(LIN_params*);
int sllin_config_down(LIN_params*);
int open_socket();
int bind_socket(LIN_params*);
int close_socket();
int receive_frame(char*);
void *send_frame(void*);

#endif // LIN_UTILS_H
