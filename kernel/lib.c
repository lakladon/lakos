#include "include/lib.h"

void strcpy(char* dest, const char* src) {
    while (*src) *dest++ = *src++;
    *dest = '\0';
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int strncmp(const char* s1, const char* s2, unsigned int n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++; s2++; n--;
    }
    if (n == 0) return 0;
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int strlen(const char* s) {
    int len = 0;
    while (*s++) len++;
    return len;
}

int atoi(const char* s) {
    int n = 0;
    while (*s >= '0' && *s <= '9') {
        n = n * 10 + (*s - '0');
        s++;
    }
    return n;
}

void* memcpy(void* dest, const void* src, unsigned int n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    while (n--) *d++ = *s++;
    return dest;
}

void* memset(void* s, int c, unsigned int n) {
    unsigned char* p = (unsigned char*)s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}

char* strstr(const char* haystack, const char* needle) {
    if (!*needle) return (char*)haystack;
    for (; *haystack; haystack++) {
        const char* h = haystack;
        const char* n = needle;
        while (*h && *n && *h == *n) { h++; n++; }
        if (!*n) return (char*)haystack;
    }
    return 0;
}

char* strncat(char* dest, const char* src, int n) {
    char* d = dest;
    while (*d) d++;
    while (n-- && *src) *d++ = *src++;
    *d = '\0';
    return dest;
}

char* strchr(const char* s, int c) {
    while (*s) {
        if (*s == c) return (char*)s;
        s++;
    }
    return 0;
}

char* strcat(char* dest, const char* src) {
    char* d = dest;
    while (*d) d++;
    while (*src) *d++ = *src++;
    *d = '\0';
    return dest;
}

void itoa(int n, char* buf) {
    if (n == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    int i = 0;
    int sign = n < 0 ? -1 : 1;
    unsigned int num = n < 0 ? -n : n;
    while (num > 0) {
        buf[i++] = '0' + num % 10;
        num /= 10;
    }
    if (sign < 0) buf[i++] = '-';
    buf[i] = '\0';
    // reverse
    for (int j = 0; j < i/2; j++) {
        char t = buf[j];
        buf[j] = buf[i-1-j];
        buf[i-1-j] = t;
    }
}