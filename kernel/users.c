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
        // Create default users when no ATA drive is available
        strcpy(users[0].username, "root");
        strcpy(users[0].password, "root");
        users[0].uid = 0;
        users[0].gid = 0;
        user_count = 1;
        // Debug output
        extern void terminal_writestring(const char*);
        terminal_writestring("DEBUG: Created default root user (ATA not detected)\n");
        return;
    }
    uint16_t buffer[256];
    ata_read_sector(0, USER_DATA_LBA, buffer);
    
    // Debug: Print raw data from sector
    extern void terminal_writestring(const char*);
    terminal_writestring("DEBUG: Raw sector data (first 16 bytes): ");
    for (int i = 0; i < 8; i++) {
        char buf[8];
        itoa(buffer[i], buf);
        terminal_writestring(buf);
        terminal_writestring(" ");
    }
    terminal_writestring("\n");
    
    memcpy(&user_count, buffer, sizeof(int));
    terminal_writestring("DEBUG: Read user_count = ");
    char buf[16];
    itoa(user_count, buf);
    terminal_writestring(buf);
    terminal_writestring("\n");
    
    if (user_count > MAX_USERS) user_count = MAX_USERS;
    
    // Debug: Print raw user data
    terminal_writestring("DEBUG: Raw user data: ");
    char* raw_data = (char*)buffer + sizeof(int);
    for (int i = 0; i < sizeof(user_t) * user_count && i < 64; i++) {
        if (raw_data[i] >= 32 && raw_data[i] < 127) {
            terminal_putchar(raw_data[i]);
        } else {
            terminal_writestring(".");
        }
    }
    terminal_writestring("\n");
    
    memcpy(users, raw_data, sizeof(user_t) * user_count);
    
    // Debug: Print loaded users
    for (int i = 0; i < user_count; i++) {
        terminal_writestring("DEBUG: Loaded User ");
        itoa(i, buf);
        terminal_writestring(buf);
        terminal_writestring(": username='");
        terminal_writestring(users[i].username);
        terminal_writestring("' password='");
        terminal_writestring(users[i].password);
        terminal_writestring("' uid=");
        itoa(users[i].uid, buf);
        terminal_writestring(buf);
        terminal_writestring(" gid=");
        itoa(users[i].gid, buf);
        terminal_writestring(buf);
        terminal_writestring("\n");
    }
    
    terminal_writestring("DEBUG: Loaded users from ATA drive\n");
}

void save_users() {
    if (!ata_identify(0)) return;
    uint16_t buffer[256] = {0};
    memcpy(buffer, &user_count, sizeof(int));
    memcpy((char*)buffer + sizeof(int), users, sizeof(user_t) * MAX_USERS);
    ata_write_sector(0, USER_DATA_LBA, buffer);
}

// Initialize default users
void create_default_users() {
    strcpy(users[0].username, "root");
    strcpy(users[0].password, "root");
    users[0].uid = 0;
    users[0].gid = 0;
    user_count = 1;
    save_users();
}

// Ensure we have valid users, create defaults if needed
void ensure_valid_users() {
    extern void terminal_writestring(const char*);
    terminal_writestring("DEBUG: Checking for valid users\n");
    
    // Check if we have any valid users
    int has_valid_users = 0;
    for (int i = 0; i < user_count; i++) {
        // Check if username is valid (not empty and contains only printable characters)
        int is_valid = 1;
        if (users[i].username[0] == '\0') {
            is_valid = 0;
        } else {
            for (int j = 0; j < 32 && users[i].username[j] != '\0'; j++) {
                if (users[i].username[j] < 32 || users[i].username[j] > 126) {
                    is_valid = 0;
                    break;
                }
            }
        }
        
        // Also check password
        if (is_valid) {
            for (int j = 0; j < 32 && users[i].password[j] != '\0'; j++) {
                if (users[i].password[j] < 32 || users[i].password[j] > 126) {
                    is_valid = 0;
                    break;
                }
            }
        }
        
        if (is_valid) {
            has_valid_users = 1;
            terminal_writestring("DEBUG: Found valid user: ");
            terminal_writestring(users[i].username);
            terminal_writestring("\n");
            break;
        }
    }
    
    if (!has_valid_users) {
        terminal_writestring("DEBUG: No valid users found, creating default root user\n");
        create_default_users();
    } else {
        terminal_writestring("DEBUG: Valid users already exist\n");
    }
}

void init_users() {
    load_users();
    // Debug output
    extern void terminal_writestring(const char*);
    char buf[16];
    terminal_writestring("DEBUG: init_users() - user_count after load = ");
    itoa(user_count, buf);
    terminal_writestring(buf);
    terminal_writestring("\n");
    
    // Check if we have any valid users
    int has_valid_users = 0;
    for (int i = 0; i < user_count; i++) {
        // Check if username is valid (not empty and contains only printable characters)
        int is_valid = 1;
        if (users[i].username[0] == '\0') {
            is_valid = 0;
        } else {
            for (int j = 0; j < 32 && users[i].username[j] != '\0'; j++) {
                if (users[i].username[j] < 32 || users[i].username[j] > 126) {
                    is_valid = 0;
                    break;
                }
            }
        }
        
        // Also check password
        if (is_valid) {
            for (int j = 0; j < 32 && users[i].password[j] != '\0'; j++) {
                if (users[i].password[j] < 32 || users[i].password[j] > 126) {
                    is_valid = 0;
                    break;
                }
            }
        }
        
        if (is_valid) {
            has_valid_users = 1;
            terminal_writestring("DEBUG: Found valid user: ");
            terminal_writestring(users[i].username);
            terminal_writestring("\n");
            break;
        }
    }
    
    if (!has_valid_users) {
        terminal_writestring("DEBUG: No valid users found, creating default root user\n");
        create_default_users();
    } else {
        terminal_writestring("DEBUG: Valid users already exist\n");
    }
    
    terminal_writestring("DEBUG: init_users() - Users initialization complete\n");
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
    // Debug: Print user count and available users
    extern void terminal_writestring(const char*);
    char buf[16];
    
    terminal_writestring("DEBUG: user_count = ");
    itoa(user_count, buf);
    terminal_writestring(buf);
    terminal_writestring("\n");
    
    for (int i = 0; i < user_count; i++) {
        terminal_writestring("DEBUG: User ");
        itoa(i, buf);
        terminal_writestring(buf);
        terminal_writestring(": ");
        terminal_writestring(users[i].username);
        terminal_writestring("\n");
    }
    
    // First check if user exists
    int user_index = -1;
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            user_index = i;
            break;
        }
    }
    
    // User not found
    if (user_index == -1) {
        terminal_writestring("DEBUG: User not found\n");
        return 0;
    }
    
    terminal_writestring("DEBUG: User found, checking password\n");
    
    // Check password
    if (strcmp(users[user_index].password, password) == 0) {
        strcpy(current_user, username);
        terminal_writestring("DEBUG: Password correct\n");
        return 1;
    } else {
        terminal_writestring("DEBUG: Password incorrect\n");
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
