// Calculator command for Lakos OS shell
// Supports: +, -, *, /, parentheses, integers and floating point

static char* calc_expr;
static int calc_pos;

static float calc_parse_expr();

static void calc_skip_spaces() {
    while (calc_expr[calc_pos] == ' ') calc_pos++;
}

static float calc_parse_number() {
    calc_skip_spaces();
    
    int negative = 0;
    if (calc_expr[calc_pos] == '-') {
        negative = 1;
        calc_pos++;
        calc_skip_spaces();
    }
    
    float result = 0;
    
    // Parse integer part
    while (calc_expr[calc_pos] >= '0' && calc_expr[calc_pos] <= '9') {
        result = result * 10 + (calc_expr[calc_pos] - '0');
        calc_pos++;
    }
    
    // Parse decimal part
    if (calc_expr[calc_pos] == '.') {
        calc_pos++;
        float decimal = 0.1f;
        while (calc_expr[calc_pos] >= '0' && calc_expr[calc_pos] <= '9') {
            result += (calc_expr[calc_pos] - '0') * decimal;
            decimal *= 0.1f;
            calc_pos++;
        }
    }
    
    return negative ? -result : result;
}

static float calc_parse_factor() {
    calc_skip_spaces();
    
    if (calc_expr[calc_pos] == '(') {
        calc_pos++;
        float result = calc_parse_expr();
        calc_skip_spaces();
        if (calc_expr[calc_pos] == ')') {
            calc_pos++;
        }
        return result;
    }
    
    return calc_parse_number();
}

static float calc_parse_term() {
    float result = calc_parse_factor();
    
    while (1) {
        calc_skip_spaces();
        char op = calc_expr[calc_pos];
        
        if (op == '*' || op == '/') {
            calc_pos++;
            float right = calc_parse_factor();
            
            if (op == '*') {
                result *= right;
            } else {
                if (right == 0) {
                    terminal_writestring("Error: division by zero\n");
                    return 0;
                }
                result /= right;
            }
        } else {
            break;
        }
    }
    
    return result;
}

static float calc_parse_expr() {
    float result = calc_parse_term();
    
    while (1) {
        calc_skip_spaces();
        char op = calc_expr[calc_pos];
        
        if (op == '+' || op == '-') {
            calc_pos++;
            float right = calc_parse_term();
            
            if (op == '+') {
                result += right;
            } else {
                result -= right;
            }
        } else {
            break;
        }
    }
    
    return result;
}

static void calc_ftoa(float value, char* buffer) {
    if (value == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }
    
    int negative = 0;
    if (value < 0) {
        negative = 1;
        value = -value;
    }
    
    // Extract integer part
    int int_part = (int)value;
    float frac_part = value - (float)int_part;
    
    // Convert integer part
    char int_str[16];
    int i = 0;
    if (int_part == 0) {
        int_str[i++] = '0';
    } else {
        while (int_part > 0) {
            int_str[i++] = '0' + (int_part % 10);
            int_part /= 10;
        }
    }
    
    int j = 0;
    if (negative) {
        buffer[j++] = '-';
    }
    while (i > 0) {
        buffer[j++] = int_str[--i];
    }
    
    // Add decimal part if non-zero
    if (frac_part > 0.0001f) {
        buffer[j++] = '.';
        
        // Up to 4 decimal places
        for (int d = 0; d < 4 && frac_part > 0.0001f; d++) {
            frac_part *= 10;
            int digit = (int)frac_part;
            buffer[j++] = '0' + digit;
            frac_part -= digit;
        }
        
        // Remove trailing zeros
        while (j > 0 && buffer[j-1] == '0') {
            j--;
        }
        // Remove trailing dot
        if (j > 0 && buffer[j-1] == '.') {
            j--;
        }
    }
    
    buffer[j] = '\0';
}

static void cmd_calc(const char* args) {
    if (!args || strlen(args) == 0) {
        terminal_writestring("Usage: calc <expression>\n");
        terminal_writestring("Examples:\n");
        terminal_writestring("  calc 2+2\n");
        terminal_writestring("  calc 10*5-3\n");
        terminal_writestring("  calc (2+3)*4\n");
        terminal_writestring("  calc 3.14*2\n");
        terminal_writestring("  calc 10/4\n");
        terminal_writestring("Operators: + - * / ( )\n");
        terminal_writestring("Supports integers and floating point numbers\n");
        return;
    }
    
    calc_expr = (char*)args;
    calc_pos = 0;
    
    float result = calc_parse_expr();
    
    char buffer[32];
    calc_ftoa(result, buffer);
    
    terminal_writestring(args);
    terminal_writestring(" = ");
    terminal_writestring(buffer);
    terminal_writestring("\n");
}