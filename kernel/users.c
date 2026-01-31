#include "include/users.h"
#include <stdint.h>

extern void* memcpy(void* dest, const void* src, unsigned int n);

user_t users[MAX_USERS];
int user_count = 0;
char current_user[32] = "";

#include "include/lib.h"

extern void ata_read_sector(uint8_t drive, uint32_t lba, uint16_t* buffer);
extern void ata_write_sector(uint8_t drive, uint32_t lba, uint16_t* buffer);
extern int ata_identify(uint8_t drive);

#define USER_DATA_LBA 100

void load_users() {
    if (!ata_identify(0)) {
        user_count = 0;
        return;
    }
    uint16_t buffer[256];
    ata_read_sector(0, USER_DATA_LBA, buffer);
    memcpy(&user_count, buffer, sizeof(int));
    if (user_count > MAX_USERS) user_count = MAX_USERS;
    memcpy(users, (char*)buffer + sizeof(int), sizeof(user_t) * user_count);
}

void save_users() {
    if (!ata_identify(0)) return;
    uint16_t buffer[256] = {0};
    memcpy(buffer, &user_count, sizeof(int));
    memcpy((char*)buffer + sizeof(int), users, sizeof(user_t) * MAX_USERS);
    ata_write_sector(0, USER_DATA_LBA, buffer);
}

void init_users() {
    load_users();
    if (user_count == 0 || user_count > MAX_USERS) {
        strcpy(users[0].username, "root");
        strcpy(users[0].password, "root");
        users[0].uid = 0;
        users[0].gid = 0;
        user_count = 1;
        save_users();
    }
}

int add_user(const char* username, const char* password) {
    if (user_count >= MAX_USERS) return 0;
    if (strlen(username) >= 32 || strlen(password) >= 32) return 0;
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0) return 0; // already exists
    }
    strcpy(users[user_count].username, username);
    strcpy(users[user_count].password, password);
    users[user_count].uid = user_count + 1; // root 0, first user 1
    users[user_count].gid = 100; // users group
    user_count++;
    return 1;
}

int authenticate_user(const char* username, const char* password) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0 && strcmp(users[i].password, password) == 0) {
            strcpy(current_user, username);
            return 1;
        }
    }
    return 0;
}

int change_password(const char* username, const char* old_password, const char* new_password) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            if (strcmp(users[i].password, old_password) == 0) {
                strcpy(users[i].password, new_password);
                return 1;
            }
            return 0; // wrong old password
        }
    }
    return 0; // user not found
}

int delete_user(const char* username) {
    if (strcmp(username, "root") == 0) return 0; // can't delete root
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            for (int j = i; j < user_count - 1; j++) {
                users[j] = users[j+1];
            }
            user_count--;
            if (strcmp(current_user, username) == 0) {
                strcpy(current_user, "root");
            }
            return 1;
        }
    }
    return 0;
}

int get_current_uid() {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, current_user) == 0) {
            return users[i].uid;
        }
    }
    return -1;
}

int get_current_gid() {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, current_user) == 0) {
            return users[i].gid;
        }
    }
    return -1;
}