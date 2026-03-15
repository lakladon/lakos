/*
 * Lakos OS
 * Copyright (c) 2026 lakladon
 * Created: January 11, 2026
 */

#include "include/crypt.h"

void encrypt_password(const char* pass, const char* key, char* output) {
    int pass_len = 0;
    while (pass[pass_len] && pass_len < MAX_PASS_LEN - 1) pass_len++;
    int key_len = 0;
    while (key[key_len]) key_len++;
    for (int i = 0; i < pass_len; i++) {
        output[i] = pass[i] ^ key[i % key_len];
    }
    output[pass_len] = '\0';
}

void decrypt_password(const char* enc, const char* key, char* output) {
    encrypt_password(enc, key, output); // XOR is symmetric
}