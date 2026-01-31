#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Simple calculator for Lakos OS
// Supports basic arithmetic operations: +, -, *, /, %, ^
// Supports parentheses for grouping
// Supports memory functions: M+, M-, MR, MC
// Supports history

#define MAX_INPUT 256
#define MAX_HISTORY 10
#define MAX_STACK 50

static double memory = 0.0;
static char history[MAX_HISTORY][MAX_INPUT];
static int history_count = 0;

// Stack for expression evaluation
typedef struct {
    double values[MAX_STACK];
    int top;
} Stack;

void stack_init(Stack* s) {
    s->top = -1;
}

int stack_empty(Stack* s) {
    return s->top == -1;
}

int stack_full(Stack* s) {
    return s->top == MAX_STACK - 1;
}

void stack_push(Stack* s, double value) {
    if (!stack_full(s)) {
        s->values[++(s->top)] = value;
    }
}

double stack_pop(Stack* s) {
    if (!stack_empty(s)) {
        return s->values[(s->top)--];
    }
    return 0.0;
}

double stack_peek(Stack* s) {
    if (!stack_empty(s)) {
        return s->values[s->top];
    }
    return 0.0;
}

// Helper functions
int is_operator(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '^';
}

int precedence(char op) {
    switch (op) {
        case '+':
        case '-':
            return 1;
        case '*':
        case '/':
        case '%':
            return 2;
        case '^':
            return 3;
        default:
            return 0;
    }
}

int is_left_associative(char op) {
    return op != '^'; // ^ is right associative
}

// Convert infix to postfix
int infix_to_postfix(const char* infix, char* postfix) {
    Stack operators;
    stack_init(&operators);
    
    int i = 0, j = 0;
    char token;
    
    while ((token = infix[i]) != '\0') {
        if (isspace(token)) {
            i++;
            continue;
        }
        
        if (isdigit(token) || token == '.') {
            // Number
            while (isdigit(infix[i]) || infix[i] == '.') {
                postfix[j++] = infix[i++];
            }
            postfix[j++] = ' ';
            continue;
        }
        
        if (token == '(') {
            stack_push(&operators, '(');
        } else if (token == ')') {
            while (!stack_empty(&operators) && stack_peek(&operators) != '(') {
                postfix[j++] = (char)stack_pop(&operators);
                postfix[j++] = ' ';
            }
            if (!stack_empty(&operators)) {
                stack_pop(&operators); // Remove '('
            }
        } else if (is_operator(token)) {
            while (!stack_empty(&operators) && 
                   stack_peek(&operators) != '(' &&
                   (precedence((char)stack_peek(&operators)) > precedence(token) ||
                    (precedence((char)stack_peek(&operators)) == precedence(token) && 
                     is_left_associative(token)))) {
                postfix[j++] = (char)stack_pop(&operators);
                postfix[j++] = ' ';
            }
            stack_push(&operators, token);
        }
        
        i++;
    }
    
    while (!stack_empty(&operators)) {
        postfix[j++] = (char)stack_pop(&operators);
        postfix[j++] = ' ';
    }
    
    postfix[j] = '\0';
    return 1;
}

// Evaluate postfix expression
double evaluate_postfix(const char* postfix) {
    Stack values;
    stack_init(&values);
    
    int i = 0;
    char token;
    
    while ((token = postfix[i]) != '\0') {
        if (isspace(token)) {
            i++;
            continue;
        }
        
        if (isdigit(token) || token == '.') {
            // Number
            double num = 0.0;
            int decimal_found = 0;
            double decimal_multiplier = 0.1;
            
            while (isdigit(postfix[i]) || postfix[i] == '.') {
                if (postfix[i] == '.') {
                    decimal_found = 1;
                } else {
                    if (decimal_found) {
                        num += (postfix[i] - '0') * decimal_multiplier;
                        decimal_multiplier *= 0.1;
                    } else {
                        num = num * 10.0 + (postfix[i] - '0');
                    }
                }
                i++;
            }
            stack_push(&values, num);
        } else if (is_operator(token)) {
            if (values.top < 1) {
                printf("Error: Invalid expression\n");
                return 0.0;
            }
            
            double b = stack_pop(&values);
            double a = stack_pop(&values);
            double result = 0.0;
            
            switch (token) {
                case '+':
                    result = a + b;
                    break;
                case '-':
                    result = a - b;
                    break;
                case '*':
                    result = a * b;
                    break;
                case '/':
                    if (b == 0.0) {
                        printf("Error: Division by zero\n");
                        return 0.0;
                    }
                    result = a / b;
                    break;
                case '%':
                    if (b == 0.0) {
                        printf("Error: Modulo by zero\n");
                        return 0.0;
                    }
                    result = (int)a % (int)b;
                    break;
                case '^':
                    result = 1.0;
                    for (int k = 0; k < (int)b; k++) {
                        result *= a;
                    }
                    break;
            }
            stack_push(&values, result);
        }
        
        i++;
    }
    
    if (values.top == 0) {
        return stack_pop(&values);
    } else {
        printf("Error: Invalid expression\n");
        return 0.0;
    }
}

// Memory functions
void memory_store(double value) {
    memory = value;
}

void memory_add(double value) {
    memory += value;
}

void memory_subtract(double value) {
    memory -= value;
}

double memory_recall() {
    return memory;
}

void memory_clear() {
    memory = 0.0;
}

// History functions
void add_to_history(const char* expression) {
    if (history_count < MAX_HISTORY) {
        strcpy(history[history_count], expression);
        history_count++;
    } else {
        // Shift history up
        for (int i = 0; i < MAX_HISTORY - 1; i++) {
            strcpy(history[i], history[i + 1]);
        }
        strcpy(history[MAX_HISTORY - 1], expression);
    }
}

void show_history() {
    if (history_count == 0) {
        printf("No history available\n");
        return;
    }
    
    printf("Calculation History:\n");
    for (int i = 0; i < history_count; i++) {
        printf("%d. %s\n", i + 1, history[i]);
    }
}

// Help function
void show_help() {
    printf("Lakos Calculator Help:\n");
    printf("Basic operations: +, -, *, /, %%, ^\n");
    printf("Use parentheses () for grouping\n");
    printf("Memory functions:\n");
    printf("  MS - Store current result in memory\n");
    printf("  M+ - Add current result to memory\n");
    printf("  M- - Subtract current result from memory\n");
    printf("  MR - Recall memory value\n");
    printf("  MC - Clear memory\n");
    printf("Other commands:\n");
    printf("  history - Show calculation history\n");
    printf("  help - Show this help\n");
    printf("  quit - Exit calculator\n");
    printf("Examples:\n");
    printf("  2 + 3 * 4\n");
    printf("  (5 + 3) * 2\n");
    printf("  10 / 2 + 3\n");
    printf("  2 ^ 3\n");
}

// Main calculator function
int main() {
    char input[MAX_INPUT];
    char postfix[MAX_INPUT];
    double result = 0.0;
    
    printf("Lakos Calculator\n");
    printf("Type 'help' for instructions, 'quit' to exit\n");
    printf("> ");
    
    while (fgets(input, MAX_INPUT, stdin) != NULL) {
        // Remove newline
        input[strcspn(input, "\n")] = 0;
        
        // Check for commands
        if (strcmp(input, "quit") == 0 || strcmp(input, "exit") == 0) {
            printf("Goodbye!\n");
            break;
        } else if (strcmp(input, "help") == 0) {
            show_help();
        } else if (strcmp(input, "history") == 0) {
            show_history();
        } else if (strcmp(input, "MR") == 0 || strcmp(input, "mr") == 0) {
            result = memory_recall();
            printf("Memory: %.6g\n", result);
        } else if (strcmp(input, "MC") == 0 || strcmp(input, "mc") == 0) {
            memory_clear();
            printf("Memory cleared\n");
        } else if (strcmp(input, "MS") == 0 || strcmp(input, "ms") == 0) {
            memory_store(result);
            printf("Stored %.6g in memory\n", result);
        } else if (strcmp(input, "M+") == 0 || strcmp(input, "m+") == 0) {
            memory_add(result);
            printf("Added %.6g to memory\n", result);
        } else if (strcmp(input, "M-") == 0 || strcmp(input, "m-") == 0) {
            memory_subtract(result);
            printf("Subtracted %.6g from memory\n", result);
        } else {
            // Regular expression evaluation
            if (strlen(input) == 0) {
                printf("> ");
                continue;
            }
            
            add_to_history(input);
            
            if (infix_to_postfix(input, postfix)) {
                result = evaluate_postfix(postfix);
                printf("= %.6g\n", result);
            } else {
                printf("Error: Invalid expression\n");
            }
        }
        
        printf("> ");
    }
    
    return 0;
}