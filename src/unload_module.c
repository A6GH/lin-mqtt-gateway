#include <stdio.h> // perror
#include <stdlib.h> // calloc
#include <string.h> // strncpy
#include <unistd.h> // syscall
#include <fcntl.h> // O_NONBLOCK
#include <sys/stat.h> // syscall
#include <sys/types.h> // syscall
#include <sys/syscall.h> // syscall
#include "lin_utils.h" // LIN_params
#include "ko_utils.h" // extract_module_name

#define delete_module(name, flags) syscall(__NR_delete_module, name, flags)

int unload_module(LIN_params *params)
{
	char *module_name = extract_module_name(params->module_path);
	
    if (delete_module(module_name, O_NONBLOCK) != 0){
        perror("remove_module");
        return 1;
    }
	printf("Kernel module %s unloaded\n", module_name);
	
    return 0;
}