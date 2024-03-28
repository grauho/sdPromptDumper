#ifndef PNG_PROCESSING_H 
#define PNG_PROCESSING_H

size_t pngFindChunk(FILE *fhandle, const char *chunk_target);
char pngValidate(FILE *fhandle);

#endif /* PNG_PROCESSING_H */

