static void cmd_useradd(const char* args) {
    if (get_current_uid() != 0) {
        terminal_writestring("Permission denied\n");
        return;
    }
    char username[32], password[32];
    char* space = strstr(args, " ");
    if (space) {
        int len = space - args;
        if (len >= 32) len = 31;
        for (int i = 0; i < len; i++) username[i] = args[i];
        username[len] = '\0';
        const char* pass = space + 1;
        int passlen = strlen(pass);
        if (passlen >= 32) passlen = 31;
        for (int i = 0; i < passlen; i++) password[i] = pass[i];
        password[passlen] = '\0';
        if (add_user(username, password)) {
            terminal_writestring("User added: ");
            terminal_writestring(username);
            terminal_writestring("\n");
            save_users();
        } else {
            terminal_writestring("Failed to add user\n");
        }
    } else {
        terminal_writestring("Usage: useradd <username> <password>\n");
    }
}
