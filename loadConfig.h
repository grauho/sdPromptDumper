#ifndef LOAD_DEFAULTS_H 
#define LOAD_DEFAULTS_H

#include "stiTokenizer.h"

struct cfgArguments
{
	char **model_path;
	char **lora_path;
	char **vae_path;
	char **bin_path;
	char **exe_name;
	STI_BOOL *abrv;
};

int loadDefaultFile(struct cfgArguments *args, char *alt_path);

#endif /* LOAD_DEFAULTS_H */

