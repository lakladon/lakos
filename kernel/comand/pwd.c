static void cmd_pwd(const char* args) {
    (void)args;
    terminal_writestring(current_dir);
    terminal_writestring("\n");
}
