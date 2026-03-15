static void cmd_passwd(const char* args) {
    char username[32], newpass[32];
    char* space = strstr(args, " ");
    if (space) {
        int len = space - args;
        if (len >= 32) len = 31;
        for (int i = 0; i < len; i++) username[i] = args[i];
        username[len] = '\0';
        const char* pass = space + 1;
        int passlen = strlen(pass);
        if (passlen >= 32) passlen = 31;
        for (int i = 0; i < passlen; i++) newpass[i] = pass[i];
        newpass[passlen] = '\0';
        int found = 0;
        for (int i = 0; i < user_count; i++) {
            if (strcmp(users[i].username, username) == 0) {
                strcpy(users[i].password, newpass);
                terminal_writestring("Password changed for ");
                terminal_writestring(username);
                terminal_writestring("\n");
                found = 1;
                break;
            }
        }
        if (found) save_users();
        if (!found) {
            terminal_writestring("User not found\n");
        }
    } else {
        terminal_writestring("Usage: passwd <username> <newpassword>\n");
    }
}
