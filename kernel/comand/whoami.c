static void cmd_whoami(const char* args) {
    (void)args;
    terminal_writestring(current_user);
    terminal_writestring("\n");
}
