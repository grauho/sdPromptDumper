#ifndef STI_TOKENIZER_H
#define STI_TOKENIZER_H

#include <stddef.h>

#define TOKEN_GUESS_LEN 32

#define STI_BOOL  char
#define STI_TRUE  1
#define STI_FALSE 0

enum tokenizeMode
{
	GO_TILL_NULL = 0,
	GO_TILL_LEN,
	NUM_MODES
};

struct stiToken
{
	size_t token_start;
	size_t token_end;
};

struct stiToken* stiNewTokenStack(const char *str, const size_t len, 
	const enum tokenizeMode mode, const char *delims, size_t *depth);
size_t stiSubtokenize(const char *str, const size_t len, const char *delims,
	const size_t mem, struct stiToken **stack, size_t *depth);
size_t stiTokenizeNullString(const char *str, const char *delims, 
	struct stiToken *stack, const size_t stack_len);
size_t stiTokenizeExplicitString(const char *str, const size_t str_len,
	const char *delims, struct stiToken *stack, const size_t stack_len);

#ifdef STI_INCLUDE_FILE_TOKENIZER
#include <stdio.h>

size_t stiTokenizeFile(FILE *fhandle, const long int len, const char *delims,
	struct stiToken *stack, const size_t stack_len);
#endif

#endif /* STI_TOKENIZER_H */

