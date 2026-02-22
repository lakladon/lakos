// ASM mode command for Lakos OS shell
// Allows interactive assembly code execution

// Forward declarations
static void int_to_hex(int val, char* buf);

// Simple string to integer (supports decimal and hex)
static int asm_strtoi(const char* s, int base) {
    int result = 0;
    int sign = 1;
    
    // Skip whitespace
    while (*s == ' ' || *s == '\t') s++;
    
    // Handle sign
    if (*s == '-') {
        sign = -1;
        s++;
    } else if (*s == '+') {
        s++;
    }
    
    // Auto-detect base
    if (base == 0) {
        if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
            base = 16;
            s += 2;
        } else if (s[0] == '0') {
            base = 8;
            s++;
        } else {
            base = 10;
        }
    } else if (base == 16 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2;
    }
    
    // Convert digits
    while (*s) {
        int digit = 0;
        if (*s >= '0' && *s <= '9') {
            digit = *s - '0';
        } else if (*s >= 'a' && *s <= 'f') {
            digit = *s - 'a' + 10;
        } else if (*s >= 'A' && *s <= 'F') {
            digit = *s - 'A' + 10;
        } else {
            break;
        }
        
        if (digit >= base) break;
        
        result = result * base + digit;
        s++;
    }
    
    return result * sign;
}

static int asm_mode_active = 0;
static char asm_buffer[4096];
static int asm_buffer_pos = 0;
static unsigned char code_buffer[4096];
static int code_size = 0;

// Simple x86 instruction encoding
static int encode_instruction(const char* line, unsigned char* out) {
    char instr[32];
    char args[128];
    int pos = 0;
    
    // Skip leading spaces
    while (line[pos] == ' ' || line[pos] == '\t') pos++;
    
    // Parse instruction name
    int i = 0;
    while (line[pos] && line[pos] != ' ' && line[pos] != '\t' && i < 31) {
        instr[i++] = line[pos++];
    }
    instr[i] = '\0';
    
    // Skip spaces before args
    while (line[pos] == ' ' || line[pos] == '\t') pos++;
    
    // Parse arguments
    i = 0;
    while (line[pos] && line[pos] != '\n' && i < 127) {
        args[i++] = line[pos++];
    }
    args[i] = '\0';
    
    // Remove trailing spaces from args
    while (i > 0 && (args[i-1] == ' ' || args[i-1] == '\t')) {
        args[--i] = '\0';
    }
    
    int len = 0;
    
    // NOP
    if (strcmp(instr, "nop") == 0) {
        out[len++] = 0x90;
    }
    // RET
    else if (strcmp(instr, "ret") == 0) {
        out[len++] = 0xC3;
    }
    // INT - interrupt
    else if (strcmp(instr, "int") == 0) {
        int num = asm_strtoi(args, 0);
        out[len++] = 0xCD;
        out[len++] = (unsigned char)num;
    }
    // MOV immediate to registers
    else if (strcmp(instr, "mov") == 0) {
        char* comma = strchr(args, ',');
        if (comma) {
            *comma = '\0';
            char* dest = args;
            char* src = comma + 1;
            
            // Trim spaces
            while (*dest == ' ') dest++;
            while (*src == ' ') src++;
            
            int val = asm_strtoi(src, 0);
            
            // MOV reg, imm32 (32-bit)
            if (strcmp(dest, "eax") == 0) {
                out[len++] = 0xB8;
            } else if (strcmp(dest, "ecx") == 0) {
                out[len++] = 0xB9;
            } else if (strcmp(dest, "edx") == 0) {
                out[len++] = 0xBA;
             } else if (strcmp(dest, "ebx") == 0) {
                out[len++] = 0xBB;
            } else if (strcmp(dest, "esp") == 0) {
                out[len++] = 0xBC;
            } else if (strcmp(dest, "ebp") == 0) {
                out[len++] = 0xBD;
            } else if (strcmp(dest, "esi") == 0) {
                out[len++] = 0xBE;
            } else if (strcmp(dest, "edi") == 0) {
                out[len++] = 0xBF;
            } else {
                // MOV r/m32, imm32 - more complex encoding
                terminal_writestring("Unknown register: ");
                terminal_writestring(dest);
                terminal_writestring("\n");
                return 0;
            }
            
            // Little-endian immediate
            out[len++] = val & 0xFF;
            out[len++] = (val >> 8) & 0xFF;
            out[len++] = (val >> 16) & 0xFF;
            out[len++] = (val >> 24) & 0xFF;
        } else {
            terminal_writestring("MOV requires comma-separated operands\n");
            return 0;
        }
    }
    // XOR
    else if (strcmp(instr, "xor") == 0) {
        char* comma = strchr(args, ',');
        if (comma) {
            *comma = '\0';
            char* dest = args;
            char* src = comma + 1;
            
            while (*dest == ' ') dest++;
            while (*src == ' ') src++;
            
            // XOR reg, reg
            int dest_reg = -1, src_reg = -1;
            
            if (strcmp(dest, "eax") == 0) dest_reg = 0;
            else if (strcmp(dest, "ecx") == 0) dest_reg = 1;
            else if (strcmp(dest, "edx") == 0) dest_reg = 2;
            else if (strcmp(dest, "ebx") == 0) dest_reg = 3;
            else if (strcmp(dest, "esp") == 0) dest_reg = 4;
            else if (strcmp(dest, "ebp") == 0) dest_reg = 5;
            else if (strcmp(dest, "esi") == 0) dest_reg = 6;
            else if (strcmp(dest, "edi") == 0) dest_reg = 7;
            
            if (strcmp(src, "eax") == 0) src_reg = 0;
            else if (strcmp(src, "ecx") == 0) src_reg = 1;
            else if (strcmp(src, "edx") == 0) src_reg = 2;
            else if (strcmp(src, "ebx") == 0) src_reg = 3;
            else if (strcmp(src, "esp") == 0) src_reg = 4;
            else if (strcmp(src, "ebp") == 0) src_reg = 5;
            else if (strcmp(src, "esi") == 0) src_reg = 6;
            else if (strcmp(src, "edi") == 0) src_reg = 7;
            
            if (dest_reg >= 0 && src_reg >= 0) {
                out[len++] = 0x31;
                out[len++] = 0xC0 | (src_reg << 3) | dest_reg;
            } else {
                terminal_writestring("XOR: unknown register\n");
                return 0;
            }
        }
    }
    // ADD reg, reg or ADD reg, imm32
    else if (strcmp(instr, "add") == 0) {
        char* comma = strchr(args, ',');
        if (comma) {
            *comma = '\0';
            char* dest = args;
            char* src = comma + 1;
            
            while (*dest == ' ') dest++;
            while (*src == ' ') src++;
            
            int dest_reg = -1, src_reg = -1;
            
            if (strcmp(dest, "eax") == 0) dest_reg = 0;
            else if (strcmp(dest, "ecx") == 0) dest_reg = 1;
            else if (strcmp(dest, "edx") == 0) dest_reg = 2;
            else if (strcmp(dest, "ebx") == 0) dest_reg = 3;
            else if (strcmp(dest, "esp") == 0) dest_reg = 4;
            else if (strcmp(dest, "ebp") == 0) dest_reg = 5;
            else if (strcmp(dest, "esi") == 0) dest_reg = 6;
            else if (strcmp(dest, "edi") == 0) dest_reg = 7;
            
            // Check if src is a register
            if (strcmp(src, "eax") == 0) src_reg = 0;
            else if (strcmp(src, "ecx") == 0) src_reg = 1;
            else if (strcmp(src, "edx") == 0) src_reg = 2;
            else if (strcmp(src, "ebx") == 0) src_reg = 3;
            else if (strcmp(src, "esp") == 0) src_reg = 4;
            else if (strcmp(src, "ebp") == 0) src_reg = 5;
            else if (strcmp(src, "esi") == 0) src_reg = 6;
            else if (strcmp(src, "edi") == 0) src_reg = 7;
            
            if (dest_reg >= 0 && src_reg >= 0) {
                // ADD reg, reg
                out[len++] = 0x01;
                out[len++] = 0xC0 | (src_reg << 3) | dest_reg;
            } else if (dest_reg >= 0) {
                // ADD reg, imm32 (add immediate to register)
                int val = asm_strtoi(src, 0);
                out[len++] = 0x81; // OR r/m32, imm32 opcode for ADD
                out[len++] = 0xC0 | dest_reg; // ModR/M byte
                out[len++] = val & 0xFF;
                out[len++] = (val >> 8) & 0xFF;
                out[len++] = (val >> 16) & 0xFF;
                out[len++] = (val >> 24) & 0xFF;
            } else {
                terminal_writestring("ADD: unknown register\n");
                return 0;
            }
        }
    }
    // SUB reg, reg
    else if (strcmp(instr, "sub") == 0) {
        char* comma = strchr(args, ',');
        if (comma) {
            *comma = '\0';
            char* dest = args;
            char* src = comma + 1;
            
            while (*dest == ' ') dest++;
            while (*src == ' ') src++;
            
            int dest_reg = -1, src_reg = -1;
            
            if (strcmp(dest, "eax") == 0) dest_reg = 0;
            else if (strcmp(dest, "ecx") == 0) dest_reg = 1;
            else if (strcmp(dest, "edx") == 0) dest_reg = 2;
            else if (strcmp(dest, "ebx") == 0) dest_reg = 3;
            
            if (strcmp(src, "eax") == 0) src_reg = 0;
            else if (strcmp(src, "ecx") == 0) src_reg = 1;
            else if (strcmp(src, "edx") == 0) src_reg = 2;
            else if (strcmp(src, "ebx") == 0) src_reg = 3;
            
            if (dest_reg >= 0 && src_reg >= 0) {
                out[len++] = 0x29;
                out[len++] = 0xC0 | (src_reg << 3) | dest_reg;
            } else {
                terminal_writestring("SUB: unknown register\n");
                return 0;
            }
        }
    }
    // INC reg
    else if (strcmp(instr, "inc") == 0) {
        char* reg = args;
        while (*reg == ' ') reg++;
        
        if (strcmp(reg, "eax") == 0) { out[len++] = 0x40; }
        else if (strcmp(reg, "ecx") == 0) { out[len++] = 0x41; }
        else if (strcmp(reg, "edx") == 0) { out[len++] = 0x42; }
        else if (strcmp(reg, "ebx") == 0) { out[len++] = 0x43; }
        else if (strcmp(reg, "esp") == 0) { out[len++] = 0x44; }
        else if (strcmp(reg, "ebp") == 0) { out[len++] = 0x45; }
        else if (strcmp(reg, "esi") == 0) { out[len++] = 0x46; }
        else if (strcmp(reg, "edi") == 0) { out[len++] = 0x47; }
        else {
            terminal_writestring("INC: unknown register\n");
            return 0;
        }
    }
    // DEC reg
    else if (strcmp(instr, "dec") == 0) {
        char* reg = args;
        while (*reg == ' ') reg++;
        
        if (strcmp(reg, "eax") == 0) { out[len++] = 0x48; }
        else if (strcmp(reg, "ecx") == 0) { out[len++] = 0x49; }
        else if (strcmp(reg, "edx") == 0) { out[len++] = 0x4A; }
        else if (strcmp(reg, "ebx") == 0) { out[len++] = 0x4B; }
        else if (strcmp(reg, "esp") == 0) { out[len++] = 0x4C; }
        else if (strcmp(reg, "ebp") == 0) { out[len++] = 0x4D; }
        else if (strcmp(reg, "esi") == 0) { out[len++] = 0x4E; }
        else if (strcmp(reg, "edi") == 0) { out[len++] = 0x4F; }
        else {
            terminal_writestring("DEC: unknown register\n");
            return 0;
        }
    }
    // PUSH reg
    else if (strcmp(instr, "push") == 0) {
        char* reg = args;
        while (*reg == ' ') reg++;
        
        if (strcmp(reg, "eax") == 0) { out[len++] = 0x50; }
        else if (strcmp(reg, "ecx") == 0) { out[len++] = 0x51; }
        else if (strcmp(reg, "edx") == 0) { out[len++] = 0x52; }
        else if (strcmp(reg, "ebx") == 0) { out[len++] = 0x53; }
        else if (strcmp(reg, "esp") == 0) { out[len++] = 0x54; }
        else if (strcmp(reg, "ebp") == 0) { out[len++] = 0x55; }
        else if (strcmp(reg, "esi") == 0) { out[len++] = 0x56; }
        else if (strcmp(reg, "edi") == 0) { out[len++] = 0x57; }
        else {
            // PUSH imm32
            int val = asm_strtoi(reg, 0);
            out[len++] = 0x68;
            out[len++] = val & 0xFF;
            out[len++] = (val >> 8) & 0xFF;
            out[len++] = (val >> 16) & 0xFF;
            out[len++] = (val >> 24) & 0xFF;
        }
    }
    // POP reg
    else if (strcmp(instr, "pop") == 0) {
        char* reg = args;
        while (*reg == ' ') reg++;
        
        if (strcmp(reg, "eax") == 0) { out[len++] = 0x58; }
        else if (strcmp(reg, "ecx") == 0) { out[len++] = 0x59; }
        else if (strcmp(reg, "edx") == 0) { out[len++] = 0x5A; }
        else if (strcmp(reg, "ebx") == 0) { out[len++] = 0x5B; }
        else if (strcmp(reg, "esp") == 0) { out[len++] = 0x5C; }
        else if (strcmp(reg, "ebp") == 0) { out[len++] = 0x5D; }
        else if (strcmp(reg, "esi") == 0) { out[len++] = 0x5E; }
        else if (strcmp(reg, "edi") == 0) { out[len++] = 0x5F; }
        else {
            terminal_writestring("POP: unknown register\n");
            return 0;
        }
    }
    // CALL - simplified
    else if (strcmp(instr, "call") == 0) {
        char* target = args;
        while (*target == ' ') target++;
        
        // CALL rel32 (relative call)
        int addr = asm_strtoi(target, 0);
        out[len++] = 0xE8;
        // Relative offset (will need adjustment)
        out[len++] = addr & 0xFF;
        out[len++] = (addr >> 8) & 0xFF;
        out[len++] = (addr >> 16) & 0xFF;
        out[len++] = (addr >> 24) & 0xFF;
    }
    // JMP - simplified
    else if (strcmp(instr, "jmp") == 0) {
        char* target = args;
        while (*target == ' ') target++;
        
        int addr = asm_strtoi(target, 0);
        out[len++] = 0xE9;
        out[len++] = addr & 0xFF;
        out[len++] = (addr >> 8) & 0xFF;
        out[len++] = (addr >> 16) & 0xFF;
        out[len++] = (addr >> 24) & 0xFF;
    }
    // CMP reg, reg
    else if (strcmp(instr, "cmp") == 0) {
        char* comma = strchr(args, ',');
        if (comma) {
            *comma = '\0';
            char* dest = args;
            char* src = comma + 1;
            
            while (*dest == ' ') dest++;
            while (*src == ' ') src++;
            
            int dest_reg = -1, src_reg = -1;
            
            if (strcmp(dest, "eax") == 0) dest_reg = 0;
            else if (strcmp(dest, "ecx") == 0) dest_reg = 1;
            else if (strcmp(dest, "edx") == 0) dest_reg = 2;
            else if (strcmp(dest, "ebx") == 0) dest_reg = 3;
            
            if (strcmp(src, "eax") == 0) src_reg = 0;
            else if (strcmp(src, "ecx") == 0) src_reg = 1;
            else if (strcmp(src, "edx") == 0) src_reg = 2;
            else if (strcmp(src, "ebx") == 0) src_reg = 3;
            
            if (dest_reg >= 0 && src_reg >= 0) {
                out[len++] = 0x39;
                out[len++] = 0xC0 | (src_reg << 3) | dest_reg;
            } else {
                terminal_writestring("CMP: unknown register\n");
                return 0;
            }
        }
    }
    // HLT
    else if (strcmp(instr, "hlt") == 0) {
        out[len++] = 0xF4;
    }
    // CLI - clear interrupts
    else if (strcmp(instr, "cli") == 0) {
        out[len++] = 0xFA;
    }
    // STI - set interrupts
    else if (strcmp(instr, "sti") == 0) {
        out[len++] = 0xFB;
    }
    // OUT - output to port
    else if (strcmp(instr, "out") == 0) {
        char* comma = strchr(args, ',');
        if (comma) {
            *comma = '\0';
            char* port = args;
            char* val = comma + 1;
            
            while (*port == ' ') port++;
            while (*val == ' ') val++;
            
            int port_num = asm_strtoi(port, 0);
            
            // OUT DX, AL or OUT imm8, AL
            if (strcmp(val, "al") == 0) {
                out[len++] = 0xE6;
                out[len++] = (unsigned char)port_num;
            } else if (strcmp(val, "ax") == 0) {
                out[len++] = 0xE7;
                out[len++] = (unsigned char)port_num;
            }
        }
    }
    // IN - input from port
    else if (strcmp(instr, "in") == 0) {
        char* comma = strchr(args, ',');
        if (comma) {
            *comma = '\0';
            char* dest = args;
            char* port = comma + 1;
            
            while (*dest == ' ') dest++;
            while (*port == ' ') port++;
            
            int port_num = asm_strtoi(port, 0);
            
            if (strcmp(dest, "al") == 0) {
                out[len++] = 0xE4;
                out[len++] = (unsigned char)port_num;
            } else if (strcmp(dest, "ax") == 0) {
                out[len++] = 0xE5;
                out[len++] = (unsigned char)port_num;
            }
        }
    }
    // DB - define byte (data)
    else if (strcmp(instr, "db") == 0) {
        char* val = args;
        while (*val == ' ') val++;
        
        int byte_val = 0;
        if (val[0] == '\'') {
            byte_val = val[1];
        } else {
            byte_val = asm_strtoi(val, 0);
        }
        out[len++] = (unsigned char)byte_val;
    }
    // ECHO - print text (pseudo-instruction)
    else if (strcmp(instr, "echo") == 0 || strcmp(instr, "print") == 0) {
        char* text = args;
        while (*text == ' ') text++;
        
        // Remove quotes if present
        if (text[0] == '"') {
            text++;
            int len_text = strlen(text);
            if (len_text > 0 && text[len_text - 1] == '"') {
                text[len_text - 1] = '\0';
            }
        }
        
        // Output the text directly
        terminal_writestring(text);
        terminal_writestring("\n");
        return 0;  // No code generated
    }
    // Comment or empty line
    else if (instr[0] == ';' || instr[0] == '#' || instr[0] == '\0') {
        return 0;
    }
    else {
        terminal_writestring("Unknown instruction: ");
        terminal_writestring(instr);
        terminal_writestring("\n");
        return 0;
    }
    
    return len;
}

// Execute the assembled code
static void execute_asm_code() {
    if (code_size == 0) {
        terminal_writestring("No code to execute.\n");
        return;
    }
    
    terminal_writestring("Executing ");
    char buf[16];
    itoa(code_size, buf);
    terminal_writestring(buf);
    terminal_writestring(" bytes of code...\n");
    
    // Print hex dump of code
    terminal_writestring("Code: ");
    for (int i = 0; i < code_size && i < 32; i++) {
        char hex[4];
        hex[0] = "0123456789ABCDEF"[(code_buffer[i] >> 4) & 0xF];
        hex[1] = "0123456789ABCDEF"[code_buffer[i] & 0xF];
        hex[2] = ' ';
        hex[3] = '\0';
        terminal_writestring(hex);
    }
    terminal_writestring("\n");
    
    // Copy code to a known executable location (stack is usually executable)
    // Use a simple approach: copy to stack and execute
    unsigned char exec_code[64];
    for (int i = 0; i < code_size && i < 63; i++) {
        exec_code[i] = code_buffer[i];
    }
    
    // Execute the code directly using inline assembly (64-bit)
    long result = 0;
    long saved_result = 0;
    
    __asm__ volatile (
        "push %%rax\n"         // save RAX
        "push %%rcx\n"         // save RCX
        "push %%rdx\n"         // save RDX
        "push %%rbx\n"         // save RBX
        "mov %1, %%rbx\n"      // code pointer
        "call *%%rbx\n"        // call the code
        "mov %%rax, %0\n"      // save result
        "pop %%rbx\n"          // restore RBX
        "pop %%rdx\n"          // restore RDX
        "pop %%rcx\n"          // restore RCX
        "pop %%rax\n"          // restore RAX
        : "=m"(saved_result)
        : "r"(exec_code)
        : "memory"
    );
    result = saved_result;
    
    terminal_writestring("Result (EAX): ");
    char hex_result[12];
    int_to_hex(result, hex_result);
    terminal_writestring(hex_result);
    terminal_writestring(" (");
    itoa(result, buf);
    terminal_writestring(buf);
    terminal_writestring(")\n");
}

// Helper to convert int to hex string
static void int_to_hex(int val, char* buf) {
    const char* hex_chars = "0123456789ABCDEF";
    buf[0] = '0';
    buf[1] = 'x';
    for (int i = 7; i >= 0; i--) {
        buf[2 + (7 - i)] = hex_chars[(val >> (i * 4)) & 0xF];
    }
    buf[10] = '\0';
}

// Read a line in ASM mode
static void asm_read_line(char* buffer, int max) {
    int ptr = 0;
    static int shift_pressed = 0;
    
    while (1) {
        if (inb(0x64) & 0x1) {
            uint8_t scancode = inb(0x60);
            
            // Handle shift
            if ((scancode & 0x7F) == 42 || (scancode & 0x7F) == 54) {
                shift_pressed = !(scancode & 0x80);
            }
            // Key press
            else if (!(scancode & 0x80)) {
                char c = 0;
                
                // Map scancodes
                if (scancode == 28) { // Enter
                    buffer[ptr] = '\0';
                    terminal_putchar('\n');
                    return;
                }
                else if (scancode == 14) { // Backspace
                    if (ptr > 0) {
                        ptr--;
                        terminal_putchar('\b');
                        terminal_putchar(' ');
                        terminal_putchar('\b');
                    }
                }
                else if (scancode == 1) { // Escape - exit ASM mode
                    buffer[0] = '\0';
                    asm_mode_active = 0;
                    terminal_writestring("\nExiting ASM mode.\n");
                    return;
                }
                else {
                    // Simple scancode to ASCII
                    if (scancode >= 2 && scancode <= 11) {
                        c = shift_pressed ? "!@#$%^&*()"[scancode - 2] : "1234567890"[scancode - 2];
                    } else if (scancode >= 16 && scancode <= 25) {
                        c = shift_pressed ? "QWERTYUIOP"[scancode - 16] : "qwertyuiop"[scancode - 16];
                    } else if (scancode >= 30 && scancode <= 38) {
                        c = shift_pressed ? "ASDFGHJKL"[scancode - 30] : "asdfghjkl"[scancode - 30];
                    } else if (scancode >= 44 && scancode <= 50) {
                        c = shift_pressed ? "ZXCVBNM"[scancode - 44] : "zxcvbnm"[scancode - 44];
                    } else if (scancode == 12) c = shift_pressed ? '_' : '-';
                    else if (scancode == 13) c = shift_pressed ? '+' : '=';
                    else if (scancode == 26) c = shift_pressed ? '{' : '[';
                    else if (scancode == 27) c = shift_pressed ? '}' : ']';
                    else if (scancode == 39) c = shift_pressed ? ':' : ';';
                    else if (scancode == 40) c = shift_pressed ? '"' : '\'';
                    else if (scancode == 41) c = shift_pressed ? '~' : '`';
                    else if (scancode == 43) c = shift_pressed ? '|' : '\\';
                    else if (scancode == 51) c = shift_pressed ? '<' : ',';
                    else if (scancode == 52) c = shift_pressed ? '>' : '.';
                    else if (scancode == 53) c = shift_pressed ? '?' : '/';
                    else if (scancode == 57) c = ' ';
                }
                
                if (c != 0 && ptr < max - 1) {
                    buffer[ptr++] = c;
                    terminal_putchar(c);
                }
            }
        }
    }
}

// Main ASM mode function
static void cmd_asm(const char* args) {
    terminal_writestring("\033[32m=== ASM Mode ===\033[0m\n");
    terminal_writestring("Enter x86 assembly instructions.\n");
    terminal_writestring("Commands:\n");
    terminal_writestring("  run   - Execute assembled code\n");
    terminal_writestring("  clear - Clear code buffer\n");
    terminal_writestring("  list  - Show current code\n");
    terminal_writestring("  help  - Show supported instructions\n");
    terminal_writestring("  exit  - Exit ASM mode (or press ESC)\n");
    terminal_writestring("\nSupported: mov, add, sub, xor, inc, dec, push, pop, cmp, jmp, call, ret, nop, int, hlt, cli, sti, in, out, db\n");
    terminal_writestring("Registers: eax, ecx, edx, ebx, esp, ebp, esi, edi\n");
    
    asm_mode_active = 1;
    asm_buffer_pos = 0;
    code_size = 0;
    asm_buffer[0] = '\0';
    
    char line[256];
    
    while (asm_mode_active) {
        terminal_writestring("\033[33mASM>\033[0m ");
        
        asm_read_line(line, sizeof(line));
        
        if (!asm_mode_active) break; // ESC was pressed
        
        // Skip empty lines
        if (line[0] == '\0') continue;
        
        // Check for commands
        if (strcmp(line, "exit") == 0 || strcmp(line, "quit") == 0 || strcmp(line, "q") == 0) {
            terminal_writestring("Exiting ASM mode.\n");
            asm_mode_active = 0;
            break;
        }
        else if (strcmp(line, "run") == 0 || strcmp(line, "exec") == 0) {
            // Auto-add ret if not present
            if (code_size == 0 || code_buffer[code_size-1] != 0xC3) {
                code_buffer[code_size++] = 0xC3; // ret
                terminal_writestring("Auto-added: ret (1 byte)\n");
            }
            execute_asm_code();
        }
        else if (strcmp(line, "clear") == 0) {
            code_size = 0;
            asm_buffer_pos = 0;
            asm_buffer[0] = '\0';
            terminal_writestring("Code buffer cleared.\n");
        }
        else if (strcmp(line, "list") == 0) {
            terminal_writestring("Current code (");
            char buf[16];
            itoa(code_size, buf);
            terminal_writestring(buf);
            terminal_writestring(" bytes):\n");
            terminal_writestring(asm_buffer);
        }
        else if (strcmp(line, "help") == 0) {
            terminal_writestring("\nSupported instructions:\n");
            terminal_writestring("  mov reg, imm32  - Move immediate to register\n");
            terminal_writestring("  mov reg, reg    - Move register to register\n");
            terminal_writestring("  add reg, reg    - Add registers\n");
            terminal_writestring("  sub reg, reg    - Subtract registers\n");
            terminal_writestring("  xor reg, reg    - XOR registers\n");
            terminal_writestring("  inc reg         - Increment register\n");
            terminal_writestring("  dec reg         - Decrement register\n");
            terminal_writestring("  push reg/imm    - Push to stack\n");
            terminal_writestring("  pop reg         - Pop from stack\n");
            terminal_writestring("  cmp reg, reg    - Compare registers\n");
            terminal_writestring("  jmp addr        - Jump to address\n");
            terminal_writestring("  call addr       - Call function\n");
            terminal_writestring("  ret             - Return from function\n");
            terminal_writestring("  nop             - No operation\n");
            terminal_writestring("  int num         - Software interrupt\n");
            terminal_writestring("  hlt             - Halt CPU\n");
            terminal_writestring("  cli             - Disable interrupts\n");
            terminal_writestring("  sti             - Enable interrupts\n");
            terminal_writestring("  in al, port     - Input from port\n");
            terminal_writestring("  out port, al    - Output to port\n");
            terminal_writestring("  db value        - Define byte\n");
            terminal_writestring("\nRegisters: eax, ecx, edx, ebx, esp, ebp, esi, edi\n");
        }
        else {
            // Assemble the instruction
            unsigned char encoded[16];
            int len = encode_instruction(line, encoded);
            
            if (len > 0) {
                // Add to code buffer
                if (code_size + len < (int)sizeof(code_buffer)) {
                    for (int i = 0; i < len; i++) {
                        code_buffer[code_size++] = encoded[i];
                    }
                    
                    // Add to source buffer for listing
                    strcpy(asm_buffer + asm_buffer_pos, line);
                    asm_buffer_pos += strlen(line);
                    asm_buffer[asm_buffer_pos++] = '\n';
                    asm_buffer[asm_buffer_pos] = '\0';
                    
                    terminal_writestring("Assembled: ");
                    char buf[16];
                    itoa(len, buf);
                    terminal_writestring(buf);
                    terminal_writestring(" byte(s)\n");
                } else {
                    terminal_writestring("Error: code buffer full\n");
                }
            }
        }
    }
}