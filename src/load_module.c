#include <stdio.h> // sprintf
#include <string.h> // strcat
#include <fcntl.h> // open, O_RDONLY
#include <unistd.h> // syscall
#include <sys/stat.h> // syscall, open
#include <sys/types.h> // syscall, open
#include <sys/syscall.h> // syscall
#include "lin_utils.h" // LIN_params
#include "ko_utils.h" // extract_module_name

#define finit_module(fd, param_values, flags) syscall(__NR_finit_module, fd, param_values, flags)

int load_module(LIN_params *params)
{
    int fd;
	char mod_params[17] = "baudrate=";
	char baud_rate[8] = {'\0'};
	sprintf(baud_rate,"%d",params->baud_rate);
	strcat(mod_params, baud_rate);
	
	fd = open(params->module_path, O_RDONLY);
    if (finit_module(fd, mod_params, 0) != 0){
		perror("finit_module");
		return 1;
    }
    close(fd);
	printf("Kernel module %s loaded\n", extract_module_name(params->module_path));
	
    return 0;
}