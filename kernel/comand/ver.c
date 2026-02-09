static void cmd_ver(const char* args) {
    (void)args;
    terminal_writestring("lakKERNEL ");
    terminal_writestring(KERNEL_VERSION);
    terminal_writestring(" [Kernel Mode]\n");
}
