#ifndef CRYPT_H
#define CRYPT_H

#define MAX_PASS_LEN 256

void encrypt_password(const char* pass, const char* key, char* output);
void decrypt_password(const char* enc, const char* key, char* output);

#endif