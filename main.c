#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "portopt.h"
#include "portegg.h"
#include "stiTokenizer.h"
#include "pngProcessing.h"

/* Could have defaults for these in some kind of config or ini file */
char *model_path    = NULL;
char *lora_path     = NULL;
char *vae_path      = NULL;
char *bin_path      = NULL;
STI_BOOL abrv_flags = STI_FALSE;

typedef void (PrintFunc)(const char *str, const size_t len);

static void printLen(const char *str, const size_t len);
static void printQuote(const char *str, const size_t len);
static void printSize(const char *str, const size_t len);
static void printModel(const char *str, const size_t len);
static void printVAE(const char *str, const size_t len);
static void printLoRA(const char *str, const size_t len);

struct paramHashNode
{
	const char* encode_name;
	const char* switch_name;
	const char abrv;
	PrintFunc *print;
	struct paramHashNode *next;
} param_nodes[] =
{
	{"parameters",      "--prompt",          'p', printQuote, NULL},
	{"Negative prompt", "--negative-prompt", 'n', printQuote, NULL},
	{"Steps",           "--steps",            0,  printLen,   NULL},
	{"CFG scale",       "--cfg-scale",        0,  printLen,   NULL},
	{"Seed",            "--seed",            's', printLen,   NULL},
	{"Size",            NULL,                 0,  printSize,  NULL},
	{"RNG",             "--rng",              0,  printLen,   NULL},
	{"Sampler",         "--sampling-method",  0,  printLen,   NULL},
	{"Model",           "--model",           'm', printModel, NULL},
	/* Below are not currently encoded values but could be in future */
	{"Width",           "--width",           'W', printLen,   NULL},
	{"Height",          "--height",          'H', printLen,   NULL},
	{"VAE path",        "--vae",              0,  printVAE,   NULL},
	{"LoRA path",       "--lora-model-dir",   0,  printLoRA,  NULL}
};

#define PARAM_HASH_NODES ((sizeof(param_nodes)) / (sizeof(param_nodes[0])))
#define PARAM_TABLE_LEN (PARAM_HASH_NODES >> 1)

static struct paramHashNode *param_table[PARAM_TABLE_LEN] = {0};

static const char* chomp(const char *str)
{
	for (; (str != NULL) && (*str == ' ' || *str == '\t'); str++);

	return str;
}

/* Chomped substring print but it's important to increment i so just using
 * the above chomp function wouldn't be enough */
static void printLen(const char *str, const size_t len)
{
	size_t i;

	if (str == NULL)
	{
		return;
	}

	for (i = 0; (i < len) && (str[i] == ' ' || str[i] == '\t'); i++);

	for (; i < len; i++)
	{
		fputc(str[i], stdout);	
	}
}

static void printQuote(const char *str, const size_t len)
{
	fputc('"', stdout);
	printLen(str, len);
	fputc('"', stdout);
}

static void printModel(const char *str, const size_t len)
{
	if (model_path != NULL)
	{
		fputs(model_path, stdout);
	}

	printLen(str, len);

	return;
}

/* XXX: Won't be called as the encoded name is not in use */
static void printVAE(const char *str, const size_t len)
{
	(void) str;
	(void) len;

	return;
}

/* XXX: Won't be called as the encoded name is not in use */
static void printLoRA(const char *str, const size_t len)
{
	(void) str;
	(void) len;

	return;
}

static void printSize(const char *str, const size_t len)
{
	size_t tmp;

	if (str != NULL)
	{
		for (tmp = 0; str[tmp] != 'x'; tmp++);

		fputs((abrv_flags == STI_TRUE) ? "-W " : "--width ", stdout);
		printLen(str, tmp);
		tmp++;
		fputs((abrv_flags == STI_TRUE) ? " -H " : " --height ",stdout);
		printLen(str + tmp, len - tmp);
	}

	return;
}

/* Daniel J. Bernstein hashing algorithm */
static size_t hashString(const unsigned char *str)
{
	size_t hash = 5381;
	int c;

	while ((c = *str++) != '\0')
	{
		hash = ((hash << 5) + hash) + c;
	}

	return hash;
}

static int addToParamTable(struct paramHashNode *node)
{
	size_t index = hashString((const unsigned char *) node->encode_name) 
		% PARAM_TABLE_LEN;
	struct paramHashNode *cursor = param_table[index];

	if (cursor == NULL)
	{
		param_table[index] = node;

		return 0;
	}

	for (; cursor->next != NULL; cursor = cursor->next)
	{
		/* Collision */
		if (strcmp(cursor->encode_name, node->encode_name) == 0)
		{
			return 1;
		}
	}

	cursor->next = node;

	return 0;
}

static int setupHashtable(void)
{
	size_t i;

	for (i = 0; i < PARAM_HASH_NODES; i++)
	{
		/* ie: If a collision is detected */
		if (addToParamTable(&param_nodes[i]) == 1)
		{
			return 1;
		}
	}

	/* An empty table is useless here */
	return (PARAM_HASH_NODES == 0);
}

static struct paramHashNode* hashLookup(const char *name)
{
	size_t index 
		= hashString((const unsigned char *) name) % PARAM_TABLE_LEN;
	struct paramHashNode *cursor = param_table[index];

	for (; cursor != NULL; cursor = cursor->next)
	{
		if (strcmp(cursor->encode_name, name) == 0)
		{
			return cursor;
		}
	}

	return NULL;
}

static void dumpHashtable(void)
{
	struct paramHashNode *cursor = NULL;
	size_t i;

	for (i = 0; i < PARAM_TABLE_LEN; i++)
	{
		cursor = param_table[i];

		fprintf(stdout, "%lu: ", i);

		while (cursor != NULL)
		{
			fprintf(stdout, "%s -> ", cursor->encode_name);
			cursor = cursor->next;
		}

		fprintf(stdout, "EOL\n");
	}
}

#define LABEL_LIM 63

/* Process all the tokens in FIFO order */
static void processTokens(const char *buffer, const struct stiToken *stack,
	const size_t depth)
{
	size_t i, j;

	fprintf(stdout, "%ssd ", (bin_path == NULL) ? "" : bin_path);

	for (i = 0; i < depth; i++)
	{
		const char *substr = buffer + stack[i].token_start;
		const size_t token_len 
			= stack[i].token_end - stack[i].token_start;
		char label[LABEL_LIM + 1] = {0};
		struct paramHashNode *node = NULL;

		for (j = 0; j < LABEL_LIM && substr[j] != ':'; j++)
		{
			label[j] = substr[j];
		}

		j++; /* Just skip over that colon */

		if ((j >= LABEL_LIM)
		|| ((node = hashLookup(chomp(label))) == NULL))
		{
			continue;
		}

		if (node->switch_name != NULL)
		{
			if ((abrv_flags == STI_TRUE)
			&& (node->abrv != 0))
			{
				fprintf(stdout, "-%c ", node->abrv);
			}
			else
			{
				fprintf(stdout, "%s ", node->switch_name);
			}
		}

		node->print(substr + j, token_len - j);
		fputc(' ', stdout);
	}

	if (vae_path != NULL)
	{
		fprintf(stdout, "--vae \"%s\" ", vae_path);
	}

	if (lora_path != NULL)
	{
		fprintf(stdout, "--lora-model-dir \"%s\" ", lora_path);
	}

	fputs("--color ", stdout);
	fputs((abrv_flags == STI_TRUE) 
		? "-o <REPLACE_ME>\n" : "--output <REPLACE_ME>\n", stdout);
}

static int dumpSDPrompt(FILE *fhandle)
{
	/* We treat this array as a FIFO stack of tokens */
	struct stiToken *tokens = NULL;
	size_t num_tokens = 0;
	/* Signature is quite literally "tEXt" */
	const char text_signature[] = {116, 69, 88, 116};
	char *buffer  = NULL;
	size_t i, buffer_size = 0;

	if ((buffer_size = pngFindChunk(fhandle, text_signature)) == 0)
	{
		fprintf(stderr, "Unable to find tEXt chunk\n");

		return 1;
	}

	/* +1 to accommodate the terminating null byte */
	if ((buffer = calloc(buffer_size + 1, sizeof(char))) == NULL)
	{
		fprintf(stderr, "Calloc error, don't see that every day\n");	

		return 1;
	}

	if (fread(buffer, sizeof(char), buffer_size, fhandle) != buffer_size)
	{
		fprintf(stderr, "Unable to fill info buffer with %lu bytes\n",
			buffer_size);
		free(buffer);

		return 1;
	}

	/* Why the PNG spec deliminates with null chars I will never know */
	if (strncmp("parameters", buffer, sizeof("parameters") - 1) == 0)
	{
		buffer[sizeof("parameters") - 1] = ':';
	}

	if ((tokens = stiNewTokenStack((const char *) buffer, buffer_size, 
		GO_TILL_NULL, "\n", &num_tokens)) == NULL)
	{
		fprintf(stderr, "Failed to generate token stack for buffer\n");
		free(buffer);

		return 1;
	}

	/* The two prompt tokens can be processed as is, the misc token needs
	 * to be further tokenized by commas before being processed, it would
	 * be better if they all were delimed by newlines but that's a 
	 * potential improvement to sdcpp for future consideration */
	/* If there are no negative-prompt args it will not get encoded so it
	 * isn't enough just to call subtokenize on the third token because it
	 * might actually be the second of only two tokens. It would be nice
	 * if it started with something like "misc" or "settings" */
	for (i = 0; i < num_tokens; i++)
	{
		if (strncmp(buffer + tokens[i].token_start, "Steps", 
			sizeof("Steps") - 1) != 0)
		{
			continue;
		}

		if (stiSubtokenize(buffer, buffer_size, ",", i, &tokens, 
			&num_tokens) == 0)
		{
			fprintf(stderr, "Bad sub-tokenize\n");
			free(tokens);
			free(buffer);

			return 1;
		}
		else
		{
			break;
		}
	}

	processTokens(buffer, tokens, num_tokens);
	free(tokens); 
	free(buffer);

	return 0;
}

static void printHelp(void)
{
	fputs("stable-diffusion.cpp Prompt Dumper, sdPromptDumper:\n\n"
		"-M, --model <DIR PATH> : Path to the user's model directory\n"
		"-L, --lora  <DIR PATH> : Path to the user's LoRA directory\n"
		"-B, --bin   <DIR PATH> : Path to sd binary directory\n"
		"-V, --vae  <FILE PATH> : As above, include the file as well\n"
		"-a, --abrv             : Abbreviates switch names in output\n"
		"-e, --endian           : Prints assumed endian form, exits\n"
		"-h, --help             : Prints this help message, exits\n\n"
		"./sdPromptDumper [FLAGS]... [PNG Files]...\n\n"
		"Example:\n"
		"./sdPromptDumper --model ~/.models/ --vae ./myVAE.safetensors"
		" aLovelyCat.png\n\n", stdout);
}

int main(int argc, char **argv)
{
	const struct portoptVerboseOpt opts[] =
	{
		{'M', "model",  PORTOPT_TRUE},
		{'L', "lora",   PORTOPT_TRUE},
		{'B', "bin",    PORTOPT_TRUE},
		{'V', "vae",    PORTOPT_TRUE},
		{'a', "abrv",   PORTOPT_FALSE},
		{'e', "endian", PORTOPT_FALSE},
		{'h', "help",   PORTOPT_FALSE}
	};
	size_t num_opts = sizeof(opts) / sizeof(opts[0]);
	size_t i, ind = 0;
	int flag, num_bad_files = 0;

	if (setupHashtable() == 1)
	{
		fprintf(stderr, "Failed to initialize parameter hashtable\n");

		return 1;
	}

	while ((flag = portoptVerbose(argc, argv, opts, num_opts, &ind)) != -1)
	{
		switch (flag)
		{
			case 'M':
				model_path = portoptGetArg(argc, argv, &ind);
				break;
			case 'L':
				lora_path = portoptGetArg(argc, argv, &ind);
				break;
			case 'B':
				bin_path = portoptGetArg(argc, argv, &ind);
				break;
			case 'V':
				vae_path = portoptGetArg(argc, argv, &ind);
				break;
			case 'a':
				abrv_flags = STI_TRUE;
				break;
			case 'e':
				fputs((porteggIsLittle() == PORTEGG_TRUE)
					? "little-endian\n"
					: "Big-Endian\n", stdout);
				return 0;
			case 'h':
				printHelp();
				return 0;
			case '?':
			default: /* fallthrough */
				break;
		}
	}

	if (ind == (size_t) argc)
	{
		fputs("Please supply a file path to an image generated with "
			"stable-diffusion.cpp\nAlternatively use -h or "
			"--help for more information\n", stdout);
	}

	/* No need to try to act upon the program name, ie: argv[0] */
	for (i = (ind == 0) ? 1 : ind; i < (size_t) argc; i++)
	{
		FILE *handle = NULL;

		if ((handle = fopen(argv[i], "rb")) == NULL)
		{
			fprintf(stderr, "Error opening %s\n", argv[i]);
			num_bad_files++;

			continue;
		}

		if (pngValidate(handle) != 1)
		{
			fprintf(stderr, "\"%s\" is not a valid PNG file\n", 
				argv[i]);
			num_bad_files++;
		}
		else
		{
			fprintf(stdout, "\n%s:\n\n", argv[i]);
			num_bad_files += dumpSDPrompt(handle);	
		}

		fclose(handle);
	}

	return num_bad_files;
}

