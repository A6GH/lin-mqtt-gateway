#include <stdlib.h> // calloc
#include <string.h> // strncpy
#include <libgen.h> // basename
#include "ko_utils.h" // function prototype

char *extract_module_name(char *module_path)
{
	char *bn = basename(module_path);
    char *module_name = (char*)calloc(((int)strlen(bn)-2),sizeof(char));
	strncpy(module_name, bn, strlen(bn)-3);

	return module_name;
}