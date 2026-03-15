/*
 * Lakos OS
 * Copyright (c) 2026 lakladon
 * Created: January 11, 2026
 */

#ifndef USERS_H
#define USERS_H

#include <stdint.h>

#define MAX_USERS 5

typedef struct {
    char username[32];
    char password[32];
    int uid;
    int gid;
} user_t;

extern user_t users[MAX_USERS];
extern int user_count;
extern char current_user[32];

void init_users();
void save_users();
void load_users();
int add_user(const char* username, const char* password);
int authenticate_user(const char* username, const char* password);
int change_password(const char* username, const char* old_password, const char* new_password);
int delete_user(const char* username);
int get_current_uid();
int get_current_gid();

#endif