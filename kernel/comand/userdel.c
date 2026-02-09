static void cmd_userdel(const char* args) {
    if (get_current_uid() != 0) {
        terminal_writestring("Permission denied\n");
        return;
    }
    if (strlen(args) > 0) {
        if (delete_user(args)) {
            terminal_writestring("User deleted: ");
            terminal_writestring(args);
            terminal_writestring("\n");
            save_users();
        } else {
            terminal_writestring("Failed to delete user\n");
        }
    } else {
        terminal_writestring("Usage: userdel <username>\n");
    }
}
