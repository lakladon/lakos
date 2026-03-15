static void cmd_login(const char* args) {
    if (strlen(args) == 0) {
        terminal_writestring("Usage: login <username> <password>\n");
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

        // Trim whitespace from username before authentication
        char trimmed_username[32];
        int src = 0, dst = 0;
        while (username[src] == ' ' || username[src] == '\t') src++;
        while (username[src] != '\0') {
            if (username[src] != ' ' && username[src] != '\t') {
                trimmed_username[dst++] = username[src];
            }
            src++;
        }
        trimmed_username[dst] = '\0';

        if (authenticate_user(trimmed_username, password)) {
            terminal_writestring("Logged in as ");
            terminal_writestring(current_user);
            terminal_writestring("\n");
        } else {
            terminal_writestring("Invalid username or password\n");
        }
    } else {
        terminal_writestring("Usage: login <username> <password>\n");
    }
}
