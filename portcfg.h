/* License information at EOF */
/* Very simple key/value pair configuration parsing header, no nesting is 
 * supported but the user may accomplish this through key naming, keys should
 * be enclosed in square brackets */
#ifndef PORTCFG_H
#define PORTCFG_H

#include <stdio.h>  /* FILE, fopen, fclose, fgetc, sscanf */
#include <stddef.h> /* NULL, size_t */

/* A kludge to convert a macro defined value to a string literal at compile
 * time for use in the sscanf format string */
#define PORTCFG_MACRO_STR(x) PORTCFG_MACRO_STR_INTERNAL(x)
#define PORTCFG_MACRO_STR_INTERNAL(x) #x

#ifndef PORTCFG_LINE_MAX
#define PORTCFG_LINE_MAX 4096
#endif /* PORTCFG_LINE_MAX */

#ifndef PORTCFG_KEY_MAX
#define PORTCFG_KEY_MAX  128
#endif /* PORTCFG_KEY_MAX */

#ifndef PORTCFG_VAL_MAX 
#define PORTCFG_VAL_MAX  2048
#endif /* PORTCFG_VAL_MAX */

static const char *portcfg_fmt = "[%" PORTCFG_MACRO_STR(PORTCFG_KEY_MAX) 
	"[^]]] %*[ :=] %" PORTCFG_MACRO_STR(PORTCFG_VAL_MAX) "[^#;\n]";

typedef void (PORTCFG_CB) (const char *key, const char *val, void *data);

/* Definitely do not want to chomp newlines in this case */
static const char* portcfgChomp(const char *str)
{
	while (str != NULL) 
	{
		switch (*str)
		{
			case ' ':  /* fallthrough */
			case '\t': /* fallthrough */
			case '\b': /* fallthrough */
			case '\r':
				str++;
				break;
			default:
				return str;
		}
	}

	return str;
}

/* Returns zero when the end of file or an error is reached, after all even
 * a supposedly empty line will at least have a length of one on account of
 * having a newline character, I wish fgets had this return value */
static size_t portcfgFetchLine(char *buffer, size_t lim, FILE *fhandle)
{
	size_t len = 0;
	int tmp;

	if ((buffer != NULL) && (fhandle != NULL) && (lim != 0))
	{
		while ((len < lim - 1) && (tmp = fgetc(fhandle)) != EOF)
		{
			buffer[len++] = tmp;

			if (tmp == '\n')
			{
				break;
			}
		}

		buffer[len] = '\0';
	}

	return len;
}

/* It would be nice to allow for escaped characters */
static void portcfgProcess(FILE *fhandle, PORTCFG_CB *Callback, void *data)
{
	char line_buffer[PORTCFG_LINE_MAX]   = {0};
	char key_buffer[PORTCFG_KEY_MAX + 1] = {0};
	char val_buffer[PORTCFG_VAL_MAX + 1] = {0};
	size_t l_end = 0;

	/* Why even bother to parse if the results aren't used */
	if (Callback == NULL)
	{
		return;
	}

#ifndef PORTCFG_NO_WARN
	if (PORTCFG_KEY_MAX + PORTCFG_VAL_MAX > PORTCFG_LINE_MAX)
	{
		fprintf(stderr, "%s: Warning, Potential for key value pair "
			"length to exceed maximum line length: "
			"(%u + %u > %u)\n", __func__,
			PORTCFG_KEY_MAX, PORTCFG_VAL_MAX, PORTCFG_LINE_MAX);
	}
#endif /* PORTCFG_NO_WARN */

	while ((l_end = portcfgFetchLine(line_buffer, PORTCFG_LINE_MAX, 
		fhandle)) != 0)
	{
		const char *str = portcfgChomp(line_buffer);

		/* Fast forward FILE cursor to next line if the full line 
		 * wasn't fetched, don't try to parse incomplete lines */
		if (line_buffer[l_end - 1] != '\n')
		{
			int ch;

			while (((ch = fgetc(fhandle)) != EOF) && (ch != '\n'));
		}
		else if (((*str != '#') && (*str != ';'))
		&& (sscanf(str, portcfg_fmt, key_buffer, val_buffer) == 2))
		{
			Callback(key_buffer, val_buffer, data);
		}

	}
}

#endif /* PORTCFG_H */

/*
BSD 4-Clause License
Copyright (c) 2024, grauho <grauho@proton.me> All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.

    Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

    All advertising materials mentioning features or use of this software must
    display the following acknowledgement: This product includes software
    developed by the <copyright holder>.

    Neither the name of the <copyright holder> nor the names of its
    contributors may be used to endorse or promote products derived from this
    software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDER> AS IS AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
