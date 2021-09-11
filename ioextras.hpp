

void put_u64_stream_pref(std::ostream &fs, uint64_t uint);

uint64_t get_u64_stream_pref(std::istream &fs, bool &b_gotNull);

// #define ULL_READ_EOF 1
// #define ULL_READ_LATE_EOF 2
// #define ULL_READ_OVERFLOW 3
// #define ULL_READ_NULL 4

// void term_fputs(char *string, FILE *file);

// void pref_fputs(unsigned char *string, FILE *file);

// void putull_pref(unsigned long long num, FILE *file);

// char null_fgets(char *str, int n, FILE *stream);

// char pref_fgets(char *str, int n, FILE *stream);

// unsigned long long fgetull_pref(FILE *stream, int *feedback);

// unsigned long long fgetull_len(FILE *stream, uint8_t len, int *feedback);

