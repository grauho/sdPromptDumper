/* STI: Simple Tokenizer Implimentation */

/* At least three functions, one for null terminated strings, one for explicit
 * length buffers, and one for files. All of them are pretty similar but 
 * different enough in places to require their own functions. NullString
 * could be just a call to a strlen function and then call to ExplicitString
 * but that would require doing an extra trip through the string which doesn't
 * seem worth it just to simplify some code */

/* All of these return the number of tokens parsed, if 'stack' is NULL returns 
 * the number of tokens that would be needed to parse the entire string. Also 
 * true if the stack length is 0 */

/* The obvious limitation of this approach is that each string needs to have
 * it's own token array and one cannot just push tokens from multiple strings
 * into the same token array */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "stiTokenizer.h"

/* len may be zero if one the mode is GO_TILL_NULL */
struct stiToken* stiNewTokenStack(const char *str, const size_t len, 
	const enum tokenizeMode mode, const char *delims, size_t *depth)
{
	struct stiToken *stack 
		= malloc(sizeof(struct stiToken) * TOKEN_GUESS_LEN);

	/* string and delims will get checked in the tokenizing functions 
	 * themselves */
	if ((depth == NULL) || (stack == NULL))
	{
		if (stack != NULL)
		{
			free(stack);
		}

		return NULL;
	}

	(*depth) = (mode == GO_TILL_NULL)
		? stiTokenizeNullString(str, delims, stack, TOKEN_GUESS_LEN)
		: stiTokenizeExplicitString(str, len, delims, stack, 
			TOKEN_GUESS_LEN);

	if ((*depth) == 0)
	{
		free(stack);

		return NULL;
	}

	/* Trims excess length if the guess over-allocated while grows the
	 * array if under-allocated */
	if ((*depth) != TOKEN_GUESS_LEN)
	{
		struct stiToken *tmp 
			= realloc(stack, sizeof(struct stiToken) * (*depth));

		if (tmp == NULL)
		{
			free(stack);
			
			return NULL;
		}

		stack = tmp;
	}

	/* If array was under-allocated the tokenizing function needs to be 
	 * called again to populate the newly allocated tokens */
	if ((*depth) > TOKEN_GUESS_LEN)
	{
		const size_t tmp = (mode == GO_TILL_NULL)
			? stiTokenizeNullString(str, delims, stack, (*depth))
			: stiTokenizeExplicitString(str, len, delims, stack, 
				(*depth));

		if (tmp != (*depth))
		{
			free(stack);

			return NULL;
		}
	}

	return stack;
}

/* Returns the new depth of the stack and zero on error, I apologize for this
 * function it gets a little hairy in places */
size_t stiSubtokenize(const char *str, const size_t len, const char *delims,
	const size_t mem, struct stiToken **stack, size_t *depth) 
{
	const char *substr;
	size_t i, substr_beg, substr_len, req = 0;
	struct stiToken *tmp = NULL;

	if ((str == NULL) || (delims == NULL) || (stack == NULL) 
	|| (depth == NULL) || (mem >= *depth))
	{
		fprintf(stderr, "%s: bad args\n", __func__);

		return 0;
	}

	substr_beg = (*stack)[mem].token_start;
	substr     = str + substr_beg;
	substr_len = len - substr_beg;
	req = stiTokenizeExplicitString(substr, substr_len, delims, NULL, 0);
	tmp = realloc(*stack, sizeof(struct stiToken) * ((*depth) + req - 1));

	if (tmp == NULL)
	{
		fprintf(stderr, "%s: Bad realloc\n", __func__);

		return 0;
	}

	*stack = tmp;

	/* Have to memmove if the subtoken being expanded is not at the end
	 * of the array */
	if (mem < (*depth - 1))
	{
		memmove(&(*stack)[mem + req], &(*stack)[mem + 1], 
			((*depth) + mem) * sizeof(struct stiToken)); 
	}

	if (stiTokenizeExplicitString(substr, substr_len, delims, 
		&(*stack)[mem], req) != req)
	{
		fprintf(stderr, "%s: Bad subtoken explicit parse\n", __func__);

		return 0;
	}

	for (i = mem; i < mem + req; i++)
	{
		(*stack)[i].token_start += substr_beg;
		(*stack)[i].token_end   += substr_beg;
	}

	*depth += req - 1;

	return *depth;
}

size_t stiTokenizeNullString(const char *str, const char *delims, 
	struct stiToken *stack, const size_t stack_len)
{
	struct stiToken dummy = {0};
	size_t i, num_tokens = 0;

	if ((str == NULL) || (delims == NULL))
	{
		return 0;
	}

	dummy.token_start = 0;

	for (i = 0; str[i] != '\0'; i++)
	{
		size_t j;

		for (j = 0; delims[j] != '\0'; j++)
		{
			if (str[i] == delims[j])
			{
				dummy.token_end = i;

				if ((stack != NULL) 
				&& (num_tokens < stack_len))
				{
					stack[num_tokens] = dummy;
				}

				num_tokens++;
				dummy.token_start = i + 1;

				break;
			}
		}
	}

	dummy.token_end = i;

	if ((stack != NULL)
	&& (num_tokens < stack_len))
	{
		stack[num_tokens] = dummy;
	}

	num_tokens++;

	return num_tokens;
}

size_t stiTokenizeExplicitString(const char *str, const size_t str_len, 
	const char *delims, struct stiToken *stack, const size_t stack_len)
{
	struct stiToken dummy = {0};
	size_t i, num_tokens = 0;

	if ((str == NULL) || (delims == NULL))
	{
		return 0;
	}

	dummy.token_start = 0;

	for (i = 0; i < str_len; i++)
	{
		size_t j;

		for (j = 0; delims[j] != '\0'; j++)
		{
			if (str[i] == delims[j])
			{
				dummy.token_end = i;

				if ((stack != NULL) 
				&& (num_tokens < stack_len))
				{
					stack[num_tokens] = dummy;
				}

				num_tokens++;
				dummy.token_start = i + 1;

				break;
			}
		}
	}

	dummy.token_end = i;

	if ((stack != NULL)
	&& (num_tokens < stack_len))
	{
		stack[num_tokens] = dummy;
	}

	num_tokens++;

	return num_tokens;
}

#ifdef STI_INCLUDE_FILE_TOKENIZER

/* Unused and untested, just thought it was an interesting idea to tokenize
 * file contents in-place */
size_t stiTokenizeFile(FILE *fhandle, const long int len, const char *delims, 
	struct stiToken *stack, const size_t stack_len)
{
	struct stiToken dummy = {0};
	size_t i, num_tokens = 0;
	long int f_start;
	int tmp;

	if ((fhandle == NULL) || (delims == NULL) 
	|| ((f_start = ftell(fhandle)) == -1))
	{
		return 0;
	}

	dummy.token_start = f_start;

	for (i = 0; (i < len) && ((tmp = fgetc(fhandle)) != EOF); i++)
	{
		size_t j;

		for (j = 0; delims[j] != '\0'; j++)
		{
			if (str[i] == delims[j])
			{
				dummy.token_end = f_start + i;

				if ((stack != NULL) 
				&& (num_tokens < stack_len))
				{
					stack[num_tokens] = dummy;
				}

				num_tokens++;
				dummy.token_start = dummy.token_end + 1;

				break;
			}
		}
	}

	dummy.token_end = f_start + i;

	if ((stack != NULL)
	&& (num_tokens < stack_len))
	{
		stack[num_tokens] = dummy;
	}

	num_tokens++;
	fseek(fhandle, f_start, SEEK_SET);

	return num_tokens;
}

#endif /* STI_INCLUDE_FILE_TOKENIZER */
