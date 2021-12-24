#define ULL_READ_EOF 1
#define ULL_READ_LATE_EOF 2
#define ULL_READ_OVERFLOW 3
#define ULL_READ_NULL 4

#include <stdint.h>

void term_fputs(char *string, FILE *file);

void pref_fputs(unsigned char *string, FILE *file);

void putull_pref(uint64_t num, FILE *file);

char null_fgets(char *str, int n, FILE *stream);

char pref_fgets(char *str, int n, FILE *stream);

uint64_t fgetull_pref(FILE *stream, int *feedback);

uint64_t fgetull_len(FILE *stream, uint8_t len, int *feedback);

