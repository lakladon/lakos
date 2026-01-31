#ifndef LIB_H
#define LIB_H

#include <stdint.h>

void strcpy(char* dest, const char* src);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, unsigned int n);
int strlen(const char* s);
int atoi(const char* s);
void* memcpy(void* dest, const void* src, unsigned int n);
void* memset(void* s, int c, unsigned int n);
char* strstr(const char* haystack, const char* needle);
char* strncat(char* dest, const char* src, int n);
void itoa(int n, char* buf);

#endif