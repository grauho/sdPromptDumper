#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "loadConfig.h"
#include "portcfg.h"

#define SDPD_CONFIG_FILE "sdPromptDumper.cfg"

/* XXX: Not sure how well this will work on windows */
#ifdef _WIN32
#define SDPD_CONFIG_PATH "\\AppData\\Local\\sdPromptDumper\\" SDPD_CONFIG_FILE
const char *home_env = "%HOMEPATH%"; /* Are the percent escapes neccessary? */
#else  /* POSIX */
#define SDPD_CONFIG_PATH "/.config/sdPromptDumper/" SDPD_CONFIG_FILE
const char *home_env = "HOME";
#endif /* Platform Check */

#define SDPD_PATH_MAX 2048

static char* lazyStrdup(const char *src)
{
	char *dst = NULL;

	if (src != NULL)
	{
		const size_t len = strlen(src) + 1;

		if ((dst = malloc(sizeof(char) * (len))) != NULL)
		{
			dst = strncpy(dst, src, len);
		}
	}

	return dst;
}

/* If loading a config file for a much larger project one would want to use
 * a more sophisticated method to access these label/variable associations such
 * as a hash-table or sorting the array so it can be binary searched */
static void cfgCallback(const char *key, const char *val, void *data)
{
	struct cfgArguments *args = (struct cfgArguments *) data;
	const struct 
	{
		const char *label;
		char **dest;
	} lookup_table[] =
	{
		{"model-dir", args->model_path},
		{"lora-dir",  args->lora_path},
		{"bin-dir",   args->bin_path},
		{"vae-path",  args->vae_path},
		{"exe-name",  args->exe_name}
	};
	const size_t lookup_len 
		= sizeof(lookup_table) / sizeof(lookup_table[0]);
	size_t i;

	for (i = 0; i < lookup_len; i++)
	{
		if ((*lookup_table[i].dest == NULL)
		&& (strcmp(key, lookup_table[i].label) == 0))
		{
			*lookup_table[i].dest = lazyStrdup(val);

			return;
		}
	}

	if ((args->abrv != NULL)
	&& (strcmp(key, "abrv-bool") == 0))
	{
		*args->abrv = (strcmp(val, "TRUE") == 0) 
			? STI_TRUE : STI_FALSE;
	}
}

int loadDefaultFile(struct cfgArguments *args, char *alt_path)
{
	FILE *cfg_handle  = NULL;
	char config_path[SDPD_PATH_MAX] = {0};

	if (alt_path == NULL)
	{
		const char *home_dir = getenv(home_env);

		if (((args == NULL) || (home_dir == NULL))
		|| (strlen(home_dir) + sizeof(SDPD_CONFIG_PATH) - 1 
			>= SDPD_PATH_MAX))
		{
			return 1;
		}

		strcat(config_path, home_dir);
		strcat(config_path, SDPD_CONFIG_PATH);
	}
	else
	{
		if ((args == NULL) 
		|| (strlen(alt_path) + sizeof(SDPD_CONFIG_FILE) - 1
			>= SDPD_PATH_MAX))
		{
			return 1;	
		}

		strcat(config_path, alt_path);
		strcat(config_path, SDPD_CONFIG_FILE);
	}

	if ((cfg_handle = fopen(config_path, "rb")) != NULL)
	{
		portcfgProcess(cfg_handle, cfgCallback, args);
		fclose(cfg_handle);
	}

	return 0;
}
