static void cmd_crypt(const char* args) {
    if (args[0] == '-' && (args[1] == 'e' || args[1] == 'd') && args[2] == ' ') {
        char mode = args[1];
        const char* p = args + 3;
        char key[32];
        int k = 0;
        while (*p && *p != ' ' && k < 31) {
            key[k++] = *p++;
        }
        key[k] = '\0';
        if (*p == ' ') p++;
        char text[MAX_PASS_LEN];
        int t = 0;
        while (*p && t < MAX_PASS_LEN - 1) {
            text[t++] = *p++;
        }
        text[t] = '\0';
        char result[MAX_PASS_LEN];
        if (mode == 'e') {
            encrypt_password(text, key, result);
            terminal_writestring("Encrypted: ");
            terminal_writestring(result);
            terminal_writestring("\n");
        } else if (mode == 'd') {
            decrypt_password(text, key, result);
            terminal_writestring("Decrypted: ");
            terminal_writestring(result);
            terminal_writestring("\n");
        }
    } else {
        terminal_writestring("Usage: crypt -e <key> <password> or crypt -d <key> <encrypted>\n");
    }
}
