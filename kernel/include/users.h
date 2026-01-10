#ifndef USERS_H
#define USERS_H

#include <stdint.h>

typedef struct {
    char username[32];
    char password[32];
} user_t;

extern user_t users[10];
extern int user_count;
extern char current_user[32];

void init_users();
void save_users();
void load_users();
int add_user(const char* username, const char* password);
int authenticate_user(const char* username, const char* password);
int change_password(const char* username, const char* old_password, const char* new_password);
int delete_user(const char* username);

#endif