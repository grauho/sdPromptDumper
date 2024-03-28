#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "portegg.h"

#define CHUNK_TRAILER 4
#define TYPE_LEN      4

int pngValidate(FILE *fhandle)
{
	const unsigned char file_signature[] 
		= {137, 80, 78, 71, 13, 10, 26, 10}; 
	unsigned char tmp[8] = {0};

	if ((fhandle == NULL)
	|| (fread(&tmp, sizeof(char), 8, fhandle) != 8)
	|| (strncmp((const char *)tmp, (const char *)file_signature, 8) != 0))
	{
		return 0;
	}

	return 1;
}

/* Returns the size of the chunk, 0 on failure, strictly speaking because of
 * how the file functions are defined this could return a long int instead and
 * use -1 as an error but it's a moot point because there shouldn't be zero
 * length chunks */
size_t pngFindChunk(FILE *fhandle, const char *chunk_target)
{
	uint32_t chunk_length;
	char chunk_type[TYPE_LEN] = {0};
	const long int initial_pos = (fhandle != NULL)
		? ftell(fhandle)
		: -1;

	if ((fread(&chunk_length, sizeof(uint32_t), 1, fhandle) != 1)
	|| (fread(&chunk_type, sizeof(char), TYPE_LEN, fhandle) != TYPE_LEN))
	{
		fseek(fhandle, initial_pos, SEEK_SET);
		fprintf(stderr, "Failure to read IHDR information\n");

		return 0;
	}

	/* For whatever reason PNG files are encoded using Big-Endian. I assume
	 * because that is de facto "network byte order". On system that are 
	 * also Big-Endian this will be essentially a no-op */
	porteggBeToSysCopy(uint32_t, chunk_length, chunk_length);

	if (fseek(fhandle, chunk_length + CHUNK_TRAILER, SEEK_CUR) != 0)
	{
		fseek(fhandle, initial_pos, SEEK_SET);
		fprintf(stderr, "fseek failure, cannot seek %u bytes\n",
			chunk_length + CHUNK_TRAILER);

		return 0;
	}

	while ((fread(&chunk_length, sizeof(uint32_t), 1, fhandle) == 1)
	&& (fread(&chunk_type, sizeof(char), TYPE_LEN, fhandle) == TYPE_LEN))
	{
		porteggBeToSysCopy(uint32_t, chunk_length, chunk_length);

		if (strncmp((const char *) chunk_type,
			(const char *) chunk_target, TYPE_LEN) == 0)
		{
			return chunk_length;
		}
		else
		{
			if (fseek(fhandle, chunk_length + CHUNK_TRAILER,
				SEEK_CUR) != 0)
			{
				fseek(fhandle, initial_pos, SEEK_SET);
				fprintf(stderr, 
					"fseek failed, cannot seek %u bytes\n",
					chunk_length + CHUNK_TRAILER);

				return 0;
			}
		}
	}

	fseek(fhandle, initial_pos, SEEK_SET);

	return 0;
}

