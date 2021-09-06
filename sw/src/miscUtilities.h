#ifndef MISC_UTILITIES_H_
#define MISC_UTILITIES_H_

// TODO Do we need both of these includes?
#include <stdint.h>
#include <inttypes.h>


/* A utility function to reverse a string  */
void misc_reverse(char str[], int length);

char* misc_lltoa(int64_t num, char* str, int base);

int utility_memcmp(const int8_t* ptr1, const int8_t* ptr2, int n);
void utility_memcpy(uint8_t *dst, const uint8_t *src, int n);
int utility_strlen(const char *str);
int utility_strccpy(uint8_t *dst, const uint8_t *src, int n);

#if (defined __linux__) || (defined __WIN32) || (defined DEBUG_DECODE_DBF)

int utility_isgraph(int ch);
int utility_isprint(int ch);

int utility_lltoa(int64_t num, char* bufptr, int base, int buflen);

int64_t utility_atoll(const char* str);

#endif

#endif
