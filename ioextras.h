#define ULL_READ_NULL 4

void term_fputs(char *string, FILE *file);

void pref_fputs(unsigned char *string, FILE *file);

void putull_pref(unsigned long long num, FILE *file);

char null_fgets(char *str, int n, FILE *stream);

char pref_fgets(char *str, int n, FILE *stream);

unsigned long long fgetull_pref(FILE *stream, int *feedback);

unsigned long long fgetull_len(FILE *stream, uint8_t len, int *feedback);

