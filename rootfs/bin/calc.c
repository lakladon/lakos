// Simple calculator for Lakos OS
// This is a standalone user program that will be loaded and executed by the kernel

#include <stdint.h>

// Simple terminal output function (kernel will provide this)
extern void terminal_writestring(const char* s);
extern void terminal_putchar(char c);

// Simple input function (kernel will provide this)
extern char get_char();

// Helper function to check if character is digit
static int is_digit(char c) {
    return c >= '0' && c <= '9';
}

// Helper function to convert string to integer
static int atoi_simple(const char* str) {
    int result = 0;
    int sign = 1;
    int i = 0;
    
    // Handle sign
    if (str[0] == '-') {
        sign = -1;
        i = 1;
    }
    
    // Convert digits
    while (str[i] != '\0' && is_digit(str[i])) {
        result = result * 10 + (str[i] - '0');
        i++;
    }
    
    return result * sign;
}

// Helper function to convert integer to string
static void itoa_simple(int value, char* buffer) {
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
    
    int i = 0;
    while (value > 0) {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }
    
    if (negative) {
        buffer[i++] = '-';
    }
    
    // Reverse string
    for (int j = 0; j < i / 2; j++) {
        char temp = buffer[j];
        buffer[j] = buffer[i - 1 - j];
        buffer[i - 1 - j] = temp;
    }
    
    buffer[i] = '\0';
}

// Simple expression parser for "num op num" format
static int parse_expression(const char* expr, int* result) {
    int num1 = 0, num2 = 0;
    char op = 0;
    int i = 0;
    
    // Skip leading spaces
    while (expr[i] == ' ') i++;
    
    // Parse first number
    int start = i;
    while (is_digit(expr[i]) || expr[i] == '-') {
        i++;
    }
    
    if (i == start) return 0; // No number found
    
    char num1_str[16];
    int len = i - start;
    for (int j = 0; j < len; j++) {
        num1_str[j] = expr[start + j];
    }
    num1_str[len] = '\0';
    num1 = atoi_simple(num1_str);
    
    // Skip spaces
    while (expr[i] == ' ') i++;
    
    // Parse operator
    if (expr[i] == '+' || expr[i] == '-' || expr[i] == '*' || expr[i] == '/') {
        op = expr[i];
        i++;
    } else {
        return 0; // Invalid operator
    }
    
    // Skip spaces
    while (expr[i] == ' ') i++;
    
    // Parse second number
    start = i;
    while (is_digit(expr[i]) || expr[i] == '-') {
        i++;
    }
    
    if (i == start) return 0; // No number found
    
    char num2_str[16];
    len = i - start;
    for (int j = 0; j < len; j++) {
        num2_str[j] = expr[start + j];
    }
    num2_str[len] = '\0';
    num2 = atoi_simple(num2_str);
    
    // Calculate result
    switch (op) {
        case '+':
            *result = num1 + num2;
            break;
        case '-':
            *result = num1 - num2;
            break;
        case '*':
            *result = num1 * num2;
            break;
        case '/':
            if (num2 == 0) {
                return 0; // Division by zero
            }
            *result = num1 / num2;
            break;
        default:
            return 0;
    }
    
    return 1; // Success
}

// Main calculator function
int main() {
    terminal_writestring("Lakos Calculator\n");
    terminal_writestring("Type 'quit' to exit\n");
    terminal_writestring("Example: 10 + 5\n");
    terminal_writestring("> ");
    
    char input_buffer[256];
    
    while (1) {
        // Read input line by line
        int pos = 0;
        char c;
        
        while (1) {
            c = get_char();
            if (c == '\r' || c == '\n') {
                input_buffer[pos] = '\0';
                break;
            } else if (c == '\b' && pos > 0) {
                pos--;
                terminal_putchar('\b');
                terminal_putchar(' ');
                terminal_putchar('\b');
            } else if (c >= 32 && c < 127 && pos < 255) {
                input_buffer[pos++] = c;
                terminal_putchar(c);
            }
        }
        
        terminal_putchar('\n');
        
        // Check for quit command
        if (input_buffer[0] == 'q' && input_buffer[1] == 'u' && input_buffer[2] == 'i' && input_buffer[3] == 't') {
            terminal_writestring("Goodbye!\n");
            break;
        }
        
        // Parse and calculate
        int result;
        if (parse_expression(input_buffer, &result)) {
            char result_str[16];
            itoa_simple(result, result_str);
            terminal_writestring("= ");
            terminal_writestring(result_str);
            terminal_writestring("\n");
        } else {
            terminal_writestring("Error: Invalid expression\n");
        }
        
        terminal_writestring("> ");
    }
    
    return 0;
}
