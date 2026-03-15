#include <stdint.h>
extern void terminal_writestring(const char* s);
extern void* tar_archive;
extern void* tar_lookup(void* archive, const char* filename);
extern int tar_get_file_size(void* archive, const char* filename);
extern char current_dir[256];
extern void kernel_execute_command(const char* input);
extern int atoi(const char* s);
extern int strlen(const char* s);
extern int strcmp(const char* s1, const char* s2);
extern int strncmp(const char* s1, const char* s2, unsigned int n);
extern char* strncpy(char* dest, const char* src, unsigned int n);
extern char* strchr(const char* s, int c);
extern char* strrchr(const char* s, int c);
extern char* strstr(const char* haystack, const char* needle);
extern void strcpy(char* dest, const char* src);
extern char* strcat(char* dest, const char* src);
#define MAX_VARS 64
#define MAX_VAR_NAME 32
#define MAX_VAR_VALUE 256
#define MAX_SCRIPT_SIZE 4096
#define MAX_LINES 256
#define MAX_LINE_LEN 256
static char var_names[MAX_VARS][MAX_VAR_NAME];
static char var_values[MAX_VARS][MAX_VAR_VALUE];
static int var_count = 0;
static char script_lines[MAX_LINES][MAX_LINE_LEN];
static int script_line_count = 0;
static int find_var(const char* name) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(var_names[i], name) == 0) {
            return i;
        }
    }
    return -1;
}
static void set_var(const char* name, const char* value) {
    int idx = find_var(name);
    if (idx >= 0) {
        strncpy(var_values[idx], value, MAX_VAR_VALUE - 1);
        var_values[idx][MAX_VAR_VALUE - 1] = '\0';
    } else if (var_count < MAX_VARS) {
        strncpy(var_names[var_count], name, MAX_VAR_NAME - 1);
        var_names[var_count][MAX_VAR_NAME - 1] = '\0';
        strncpy(var_values[var_count], value, MAX_VAR_VALUE - 1);
        var_values[var_count][MAX_VAR_VALUE - 1] = '\0';
        var_count++;
    }
}
static const char* get_var(const char* name) {
    int idx = find_var(name);
    if (idx >= 0) {
        return var_values[idx];
    }
    return "";
}
static int is_alnum(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}
static int is_digit(char c) {
    return c >= '0' && c <= '9';
}
static int is_space(char c) {
    return c == ' ' || c == '\t';
}
static const char* skip_whitespace(const char* s) {
    while (is_space(*s)) s++;
    return s;
}
static void expand_variables(const char* input, char* output, int max_len) {
    int out_pos = 0;
    for (int i = 0; input[i] && out_pos < max_len - 1; i++) {
        if (input[i] == '$') {
            char var_name[MAX_VAR_NAME];
            int var_pos = 0;
            if (input[i + 1] == '{') {
                i += 2;
                while (input[i] && input[i] != '}' && var_pos < MAX_VAR_NAME - 1) {
                    var_name[var_pos++] = input[i++];
                }
                if (input[i] == '}') i++;
            } else {
                i++;
                while (is_alnum(input[i]) && var_pos < MAX_VAR_NAME - 1) {
                    var_name[var_pos++] = input[i++];
                }
                i--; 
            }
            var_name[var_pos] = '\0';
            const char* value = get_var(var_name);
            int value_len = strlen(value);
            for (int v = 0; v < value_len && out_pos < max_len - 1; v++) {
                output[out_pos++] = value[v];
            }
        } else {
            output[out_pos++] = input[i];
        }
    }
    output[out_pos] = '\0';
}
static int eval_expr(const char* expr) {
    expr = skip_whitespace(expr);
    if (is_digit(*expr) || (*expr == '-' && is_digit(expr[1]))) {
        return atoi(expr);
    }
    if (*expr == '$') {
        char var_name[MAX_VAR_NAME];
        int pos = 0;
        expr++;
        while (is_alnum(*expr) && pos < MAX_VAR_NAME - 1) {
            var_name[pos++] = *expr++;
        }
        var_name[pos] = '\0';
        return atoi(get_var(var_name));
    }
    return 0;
}
static int compare_values(const char* left, const char* op, const char* right) {
    int l = eval_expr(left);
    int r = eval_expr(right);
    if (strcmp(op, "==") == 0) return l == r;
    if (strcmp(op, "!=") == 0) return l != r;
    if (strcmp(op, "<") == 0) return l < r;
    if (strcmp(op, ">") == 0) return l > r;
    if (strcmp(op, "<=") == 0) return l <= r;
    if (strcmp(op, ">=") == 0) return l >= r;
    char left_expanded[MAX_VAR_VALUE];
    char right_expanded[MAX_VAR_VALUE];
    expand_variables(left, left_expanded, MAX_VAR_VALUE);
    expand_variables(right, right_expanded, MAX_VAR_VALUE);
    if (strcmp(op, "eq") == 0) return strcmp(left_expanded, right_expanded) == 0;
    if (strcmp(op, "ne") == 0) return strcmp(left_expanded, right_expanded) != 0;
    return 0;
}
static int parse_condition(const char* line) {
    char left[MAX_VAR_VALUE];
    char op[8];
    char right[MAX_VAR_VALUE];
    line = skip_whitespace(line);
    const char* ops[] = {"==", "!=", "<=", ">=", "<", ">", "eq", "ne"};
    int op_count = 8;
    for (int i = 0; i < op_count; i++) {
        const char* found = strstr(line, ops[i]);
        if (found) {
            int left_len = found - line;
            strncpy(left, line, left_len);
            left[left_len] = '\0';
            while (left_len > 0 && is_space(left[left_len - 1])) {
                left[--left_len] = '\0';
            }
            strcpy(op, ops[i]);
            const char* right_start = found + strlen(ops[i]);
            right_start = skip_whitespace(right_start);
            strcpy(right, right_start);
            int right_len = strlen(right);
            while (right_len > 0 && (is_space(right[right_len - 1]) || right[right_len - 1] == '\n')) {
                right[--right_len] = '\0';
            }
            return compare_values(left, op, right);
        }
    }
    return 0;
}
static void execute_line(const char* line);
static int find_matching_fi(int start_line) {
    int depth = 1;
    for (int i = start_line; i < script_line_count; i++) {
        const char* l = skip_whitespace(script_lines[i]);
        if (strncmp(l, "if ", 3) == 0) depth++;
        else if (strcmp(l, "fi") == 0 || strncmp(l, "fi\n", 3) == 0) {
            depth--;
            if (depth == 0) return i;
        }
    }
    return -1;
}
static int find_matching_done(int start_line) {
    int depth = 1;
    for (int i = start_line; i < script_line_count; i++) {
        const char* l = skip_whitespace(script_lines[i]);
        if (strncmp(l, "while ", 6) == 0) depth++;
        else if (strcmp(l, "done") == 0 || strncmp(l, "done\n", 5) == 0) {
            depth--;
            if (depth == 0) return i;
        }
    }
    return -1;
}
static int find_else(int start_line, int fi_line) {
    int depth = 1;
    for (int i = start_line; i < fi_line; i++) {
        const char* l = skip_whitespace(script_lines[i]);
        if (strncmp(l, "if ", 3) == 0) depth++;
        else if (strcmp(l, "else") == 0 || strncmp(l, "else\n", 5) == 0) {
            depth--;
            if (depth == 0) return i;
        }
        else if (strcmp(l, "fi") == 0 || strncmp(l, "fi\n", 3) == 0) depth--;
    }
    return -1;
}
static void execute_block(int start_line, int end_line) {
    for (int i = start_line; i < end_line && i < script_line_count; i++) {
        const char* line = skip_whitespace(script_lines[i]);
        if (*line == '\0' || *line == '\n' || *line == '#') continue;
        if (strncmp(line, "if ", 3) == 0) {
            int fi_line = find_matching_fi(i + 1);
            if (fi_line < 0) {
                terminal_writestring("lsh: missing fi\n");
                return;
            }
            int condition = parse_condition(line + 3);
            int else_line = find_else(i + 1, fi_line);
            if (condition) {
                int end = (else_line >= 0) ? else_line : fi_line;
                execute_block(i + 1, end);
            } else if (else_line >= 0) {
                execute_block(else_line + 1, fi_line);
            }
            i = fi_line;
            continue;
        }
        if (strncmp(line, "while ", 6) == 0) {
            int done_line = find_matching_done(i + 1);
            if (done_line < 0) {
                terminal_writestring("lsh: missing done\n");
                return;
            }
            int max_iterations = 1000; 
            int iterations = 0;
            while (parse_condition(line + 6) && iterations < max_iterations) {
                execute_block(i + 1, done_line);
                iterations++;
            }
            if (iterations >= max_iterations) {
                terminal_writestring("lsh: while loop limit reached\n");
            }
            i = done_line;
            continue;
        }
        if (strcmp(line, "else") == 0 || strncmp(line, "else\n", 5) == 0) continue;
        if (strcmp(line, "fi") == 0 || strncmp(line, "fi\n", 3) == 0) continue;
        if (strcmp(line, "done") == 0 || strncmp(line, "done\n", 5) == 0) continue;
        execute_line(line);
    }
}
static void execute_line(const char* line) {
    line = skip_whitespace(line);
    if (*line == '\0' || *line == '\n' || *line == '#') return;
    char* eq = strchr(line, '=');
    if (eq && eq != line) {
        char var_name[MAX_VAR_NAME];
        int name_len = eq - line;
        if (name_len < MAX_VAR_NAME) {
            strncpy(var_name, line, name_len);
            var_name[name_len] = '\0';
            int valid = 1;
            for (int i = 0; i < name_len; i++) {
                if (!is_alnum(var_name[i]) && var_name[i] != '_') {
                    valid = 0;
                    break;
                }
            }
            if (valid) {
                char value[MAX_VAR_VALUE];
                const char* val_start = eq + 1;
                if (*val_start == '"') {
                    val_start++;
                    char* end_quote = strchr(val_start, '"');
                    if (end_quote) {
                        int val_len = end_quote - val_start;
                        strncpy(value, val_start, val_len);
                        value[val_len] = '\0';
                    } else {
                        strcpy(value, val_start);
                    }
                } else {
                    strncpy(value, val_start, MAX_VAR_VALUE - 1);
                    value[MAX_VAR_VALUE - 1] = '\0';
                    int len = strlen(value);
                    if (len > 0 && value[len - 1] == '\n') value[len - 1] = '\0';
                }
                char expanded[MAX_VAR_VALUE];
                expand_variables(value, expanded, MAX_VAR_VALUE);
                set_var(var_name, expanded);
                return;
            }
        }
    }
    if (strncmp(line, "set ", 4) == 0) {
        const char* rest = line + 4;
        rest = skip_whitespace(rest);
        char var_name[MAX_VAR_NAME];
        int pos = 0;
        while (*rest && !is_space(*rest) && pos < MAX_VAR_NAME - 1) {
            var_name[pos++] = *rest++;
        }
        var_name[pos] = '\0';
        rest = skip_whitespace(rest);
        char value[MAX_VAR_VALUE];
        strncpy(value, rest, MAX_VAR_VALUE - 1);
        value[MAX_VAR_VALUE - 1] = '\0';
        int len = strlen(value);
        if (len > 0 && value[len - 1] == '\n') value[len - 1] = '\0';
        char expanded[MAX_VAR_VALUE];
        expand_variables(value, expanded, MAX_VAR_VALUE);
        set_var(var_name, expanded);
        return;
    }
    if (strncmp(line, "print ", 6) == 0) {
        const char* rest = line + 6;
        char expanded[MAX_VAR_VALUE];
        expand_variables(rest, expanded, MAX_VAR_VALUE);
        int len = strlen(expanded);
        if (len > 0 && expanded[len - 1] == '\n') expanded[len - 1] = '\0';
        terminal_writestring(expanded);
        terminal_writestring("\n");
        return;
    }
    if (strncmp(line, "exit", 4) == 0) {
        return;
    }
    if (strncmp(line, "sh ", 3) == 0) {
        const char* cmd = line + 3;
        cmd = skip_whitespace(cmd);
        char expanded[MAX_LINE_LEN];
        expand_variables(cmd, expanded, MAX_LINE_LEN);
        int len = strlen(expanded);
        if (len > 0 && expanded[len - 1] == '\n') expanded[len - 1] = '\0';
        if (strlen(expanded) > 0) {
            kernel_execute_command(expanded);
        }
        return;
    }
    if (strncmp(line, "sleep ", 6) == 0) {
        int delay = atoi(line + 6);
        for (volatile int i = 0; i < delay * 1000000; i++) {
            __asm__ volatile("nop");
        }
        return;
    }
    terminal_writestring("lsh: unknown command '");
    terminal_writestring(line);
    terminal_writestring("'. Use 'sh command' to execute shell commands.\n");
}
static int load_script_from_tar(const char* filename) {
    if (!tar_archive) {
        return 0;
    }
    char tar_path[256];
    if (filename[0] == '/') {
        strcpy(tar_path, filename + 1);
    } else if (strcmp(current_dir, "/") == 0) {
        strcpy(tar_path, filename);
    } else {
        strcpy(tar_path, current_dir + 1);
        if (tar_path[strlen(tar_path) - 1] != '/') {
            strcat(tar_path, "/");
        }
        strcat(tar_path, filename);
    }
    void* data = tar_lookup(tar_archive, tar_path);
    int size = tar_get_file_size(tar_archive, tar_path);
    if (!data || size <= 0) {
        return 0;
    }
    char* script = (char*)data;
    script_line_count = 0;
    int line_start = 0;
    for (int i = 0; i <= size && script_line_count < MAX_LINES; i++) {
        if (i == size || script[i] == '\n') {
            int line_len = i - line_start;
            if (line_len >= MAX_LINE_LEN) line_len = MAX_LINE_LEN - 1;
            strncpy(script_lines[script_line_count], script + line_start, line_len);
            script_lines[script_line_count][line_len] = '\0';
            script_line_count++;
            line_start = i + 1;
        }
    }
    return 1;
}
extern void read_line(char* buffer, int max, int echo);
static int is_block_start(const char* line) {
    line = skip_whitespace(line);
    return strncmp(line, "if ", 3) == 0 || strncmp(line, "while ", 6) == 0;
}
static void lsh_repl() {
    char line[MAX_LINE_LEN];
    terminal_writestring("lsh interactive mode. Type 'exit' to quit, 'help' for commands.\n");
    while (1) {
        terminal_writestring("lsh> ");
        read_line(line, MAX_LINE_LEN - 1, 1);
        const char* trimmed = skip_whitespace(line);
        if (*trimmed == '\0') continue;
        if (strcmp(trimmed, "exit") == 0 || strncmp(trimmed, "exit ", 5) == 0) {
            terminal_writestring("Exiting lsh interactive mode.\n");
            break;
        }
        if (strcmp(trimmed, "help") == 0) {
            terminal_writestring("lsh commands:\n");
            terminal_writestring("  var=value    - Set variable\n");
            terminal_writestring("  print text   - Print text ($var expanded)\n");
            terminal_writestring("  set var val  - Set variable\n");
            terminal_writestring("  sh command   - Execute shell command\n");
            terminal_writestring("  exit         - Exit interactive mode\n");
            terminal_writestring("  help         - Show this help\n");
            terminal_writestring("  if/else/fi   - Conditional\n");
            terminal_writestring("  while/done   - Loop\n");
            continue;
        }
        if (is_block_start(trimmed)) {
            script_line_count = 0;
            strncpy(script_lines[script_line_count], trimmed, MAX_LINE_LEN - 1);
            script_lines[script_line_count][MAX_LINE_LEN - 1] = '\0';
            script_line_count++;
            int if_depth = 0;
            int while_depth = 0;
            if (strncmp(trimmed, "if ", 3) == 0) if_depth = 1;
            if (strncmp(trimmed, "while ", 6) == 0) while_depth = 1;
            while (if_depth > 0 || while_depth > 0) {
                if (if_depth > 0) {
                    terminal_writestring("...> ");
                } else {
                    terminal_writestring("...> ");
                }
                read_line(line, MAX_LINE_LEN - 1, 1);
                trimmed = skip_whitespace(line);
                if (strlen(trimmed) == 0) continue;
                if (script_line_count < MAX_LINES) {
                    strncpy(script_lines[script_line_count], trimmed, MAX_LINE_LEN - 1);
                    script_lines[script_line_count][MAX_LINE_LEN - 1] = '\0';
                    script_line_count++;
                }
                if (strncmp(trimmed, "if ", 3) == 0) if_depth++;
                else if (strcmp(trimmed, "fi") == 0) if_depth--;
                else if (strncmp(trimmed, "while ", 6) == 0) while_depth++;
                else if (strcmp(trimmed, "done") == 0) while_depth--;
            }
            execute_block(0, script_line_count);
        } else {
            execute_line(trimmed);
        }
    }
}
static void cmd_lsh(const char* args) {
    args = skip_whitespace(args);
    if (*args == '\0') {
        lsh_repl();
        return;
    }
    if (strcmp(args, "-i") == 0 || strncmp(args, "-i ", 3) == 0) {
        lsh_repl();
        return;
    }
    const char* ext = strrchr(args, '.');
    if (!ext || strcmp(ext, ".lsh") != 0) {
        terminal_writestring("lsh: script file must have .lsh extension\n");
        return;
    }
    if (!load_script_from_tar(args)) {
        terminal_writestring("lsh: cannot load script '");
        terminal_writestring(args);
        terminal_writestring("'\n");
        return;
    }
    terminal_writestring("Executing: ");
    terminal_writestring(args);
    terminal_writestring("\n");
    execute_block(0, script_line_count);
    terminal_writestring("lsh: script completed\n");
}