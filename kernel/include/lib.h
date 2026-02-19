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
char* strchr(const char* s, int c);
char* strcat(char* dest, const char* src);
char* strrchr(const char* s, int c);
char* strncpy(char* dest, const char* src, unsigned int n);
void itoa(int n, char* buf);
void tar_list_directory(void* archive, const char* dirpath);
int snprintf(char* str, unsigned int size, const char* format, ...);
void terminal_capture_begin(char* buffer, int size);
void terminal_capture_end();

#endif
