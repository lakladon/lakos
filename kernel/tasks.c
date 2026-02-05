#include "include/tasks.h"
#include "include/lib.h"
#include <stddef.h>

extern void terminal_writestring(const char* s);
extern void terminal_putchar(char c);

#define MAX_TASKS 32

static task_t task_list[MAX_TASKS];
static uint16_t task_count_val = 0;
static uint16_t task_count_system_val = 0;
static uint16_t task_count_user_val = 0;
static uint16_t next_task_id = 1;
static uint32_t current_time = 0; // Simple monotonic clock

void tasks_init() {
    task_count_val = 0;
    task_count_system_val = 0;
    task_count_user_val = 0;
    next_task_id = 1;
    current_time = 0;
    
    // Initialize task array
    for (int i = 0; i < MAX_TASKS; i++) {
        task_list[i].id = 0;
        task_list[i].title[0] = '\0';
        task_list[i].description[0] = '\0';
        task_list[i].status = TASK_PENDING;
        task_list[i].priority = PRIORITY_NORMAL;
        task_list[i].type = TASK_TYPE_USER;
        task_list[i].created_time = 0;
        task_list[i].due_time = 0;
        task_list[i].completion_time = 0;
        task_list[i].progress = 0;
        task_list[i].sys_task_type = 0;
    }
}

uint16_t task_create(const char* title, const char* description, task_priority_t priority, uint32_t due_time) {
    if (task_count_val >= MAX_TASKS) {
        return 0; // Failed: task list full
    }
    
    if (title == NULL || strlen(title) == 0) {
        return 0; // Failed: invalid title
    }
    
    // Find an empty slot
    int slot = -1;
    for (int i = 0; i < MAX_TASKS; i++) {
        if (task_list[i].id == 0) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        return 0; // No empty slot
    }
    
    uint16_t new_id = next_task_id++;
    
    // Copy title (max 63 chars + null terminator)
    int title_len = strlen(title);
    if (title_len > 63) title_len = 63;
    for (int i = 0; i < title_len; i++) {
        task_list[slot].title[i] = title[i];
    }
    task_list[slot].title[title_len] = '\0';
    
    // Copy description (max 255 chars + null terminator)
    if (description != NULL) {
        int desc_len = strlen(description);
        if (desc_len > 255) desc_len = 255;
        for (int i = 0; i < desc_len; i++) {
            task_list[slot].description[i] = description[i];
        }
        task_list[slot].description[desc_len] = '\0';
    } else {
        task_list[slot].description[0] = '\0';
    }
    
    task_list[slot].id = new_id;
    task_list[slot].status = TASK_PENDING;
    task_list[slot].priority = priority;
    task_list[slot].type = TASK_TYPE_USER;
    task_list[slot].created_time = current_time;
    task_list[slot].due_time = due_time;
    task_list[slot].completion_time = 0;
    task_list[slot].progress = 0;
    task_list[slot].sys_task_type = 0;
    
    task_count_val++;
    task_count_user_val++;
    current_time++;
    
    return new_id;
}

uint16_t task_create_system(const char* title, const char* description, system_task_type_t sys_type, task_priority_t priority) {
    if (task_count_val >= MAX_TASKS) {
        return 0; // Failed: task list full
    }
    
    if (title == NULL || strlen(title) == 0) {
        return 0; // Failed: invalid title
    }
    
    // Find an empty slot
    int slot = -1;
    for (int i = 0; i < MAX_TASKS; i++) {
        if (task_list[i].id == 0) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        return 0; // No empty slot
    }
    
    uint16_t new_id = next_task_id++;
    
    // Copy title
    int title_len = strlen(title);
    if (title_len > 63) title_len = 63;
    for (int i = 0; i < title_len; i++) {
        task_list[slot].title[i] = title[i];
    }
    task_list[slot].title[title_len] = '\0';
    
    // Copy description
    if (description != NULL) {
        int desc_len = strlen(description);
        if (desc_len > 255) desc_len = 255;
        for (int i = 0; i < desc_len; i++) {
            task_list[slot].description[i] = description[i];
        }
        task_list[slot].description[desc_len] = '\0';
    } else {
        task_list[slot].description[0] = '\0';
    }
    
    task_list[slot].id = new_id;
    task_list[slot].status = TASK_PENDING;
    task_list[slot].priority = priority;
    task_list[slot].type = TASK_TYPE_SYSTEM;
    task_list[slot].sys_task_type = sys_type;
    task_list[slot].created_time = current_time;
    task_list[slot].due_time = 0;
    task_list[slot].completion_time = 0;
    task_list[slot].progress = 0;
    
    task_count_val++;
    task_count_system_val++;
    current_time++;
    
    return new_id;
}

int task_update_status(uint16_t task_id, task_status_t status) {
    for (int i = 0; i < MAX_TASKS; i++) {
        if (task_list[i].id == task_id) {
            task_list[i].status = status;
            if (status == TASK_COMPLETED) {
                task_list[i].completion_time = current_time;
                task_list[i].progress = 100;
            }
            return 1; // Success
        }
    }
    return 0; // Task not found
}

int task_delete(uint16_t task_id) {
    for (int i = 0; i < MAX_TASKS; i++) {
        if (task_list[i].id == task_id) {
            if (task_list[i].type == TASK_TYPE_SYSTEM) {
                if (task_count_system_val > 0) task_count_system_val--;
            } else {
                if (task_count_user_val > 0) task_count_user_val--;
            }
            task_list[i].id = 0;
            task_list[i].title[0] = '\0';
            task_list[i].description[0] = '\0';
            if (task_count_val > 0) task_count_val--;
            return 1; // Success
        }
    }
    return 0; // Task not found
}

int task_set_progress(uint16_t task_id, uint8_t progress) {
    if (progress > 100) progress = 100;
    
    for (int i = 0; i < MAX_TASKS; i++) {
        if (task_list[i].id == task_id) {
            task_list[i].progress = progress;
            if (progress == 100) {
                task_list[i].status = TASK_COMPLETED;
                task_list[i].completion_time = current_time;
            }
            return 1; // Success
        }
    }
    return 0; // Task not found
}

task_t* task_get(uint16_t task_id) {
    for (int i = 0; i < MAX_TASKS; i++) {
        if (task_list[i].id == task_id) {
            return &task_list[i];
        }
    }
    return NULL; // Task not found
}

int task_count() {
    return task_count_val;
}

int task_count_system() {
    return task_count_system_val;
}

int task_count_user() {
    return task_count_user_val;
}

void task_list_all() {
    if (task_count_val == 0) {
        terminal_writestring("No tasks yet.\n");
        return;
    }
    
    terminal_writestring("=== Task List ===\n");
    terminal_writestring("ID   Status    Progress  Title\n");
    terminal_writestring("---  --------  --------  -----\n");
    
    for (int i = 0; i < MAX_TASKS; i++) {
        if (task_list[i].id == 0) continue;
        
        // ID
        char id_str[8];
        itoa(task_list[i].id, id_str);
        terminal_writestring(id_str);
        terminal_writestring("   ");
        
        // Status
        const char* status_str = "";
        switch (task_list[i].status) {
            case TASK_PENDING: status_str = "Pending "; break;
            case TASK_RUNNING: status_str = "Running "; break;
            case TASK_COMPLETED: status_str = "Done    "; break;
            case TASK_PAUSED: status_str = "Paused  "; break;
            case TASK_FAILED: status_str = "Failed  "; break;
        }
        terminal_writestring(status_str);
        
        // Progress bar
        terminal_writestring("[");
        int filled = task_list[i].progress / 10;
        for (int j = 0; j < 10; j++) {
            terminal_putchar(j < filled ? '=' : '-');
        }
        terminal_writestring("] ");
        
        char progress_str[4];
        itoa(task_list[i].progress, progress_str);
        terminal_writestring(progress_str);
        terminal_writestring("%  ");
        
        // Title
        terminal_writestring(task_list[i].title);
        terminal_writestring("\n");
    }
    
    terminal_writestring("---  --------  --------  -----\n");
    terminal_writestring("Total tasks: ");
    char count_str[4];
    itoa(task_count_val, count_str);
    terminal_writestring(count_str);
    terminal_writestring("\n");
}

void task_show_details(uint16_t task_id) {
    task_t* task = task_get(task_id);
    if (!task) {
        terminal_writestring("Task not found.\n");
        return;
    }
    
    terminal_writestring("=== Task Details ===\n");
    
    terminal_writestring("ID: ");
    char id_str[8];
    itoa(task->id, id_str);
    terminal_writestring(id_str);
    terminal_writestring("\n");
    
    terminal_writestring("Title: ");
    terminal_writestring(task->title);
    terminal_writestring("\n");
    
    terminal_writestring("Description: ");
    if (strlen(task->description) > 0) {
        terminal_writestring(task->description);
    } else {
        terminal_writestring("(none)");
    }
    terminal_writestring("\n");
    
    terminal_writestring("Type: ");
    if (task->type == TASK_TYPE_SYSTEM) {
        terminal_writestring("System");
    } else {
        terminal_writestring("User");
    }
    terminal_writestring("\n");
    
    terminal_writestring("Status: ");
    const char* status_str = "";
    switch (task->status) {
        case TASK_PENDING: status_str = "Pending"; break;
        case TASK_RUNNING: status_str = "Running"; break;
        case TASK_COMPLETED: status_str = "Completed"; break;
        case TASK_PAUSED: status_str = "Paused"; break;
        case TASK_FAILED: status_str = "Failed"; break;
    }
    terminal_writestring(status_str);
    terminal_writestring("\n");
    
    terminal_writestring("Priority: ");
    const char* priority_str = "";
    switch (task->priority) {
        case PRIORITY_LOW: priority_str = "Low"; break;
        case PRIORITY_NORMAL: priority_str = "Normal"; break;
        case PRIORITY_HIGH: priority_str = "High"; break;
        case PRIORITY_CRITICAL: priority_str = "Critical"; break;
    }
    terminal_writestring(priority_str);
    terminal_writestring("\n");
    
    terminal_writestring("Progress: ");
    char progress_str[4];
    itoa(task->progress, progress_str);
    terminal_writestring(progress_str);
    terminal_writestring("%\n");
}

void task_list_system() {
    int sys_count = 0;
    for (int i = 0; i < MAX_TASKS; i++) {
        if (task_list[i].id != 0 && task_list[i].type == TASK_TYPE_SYSTEM) {
            sys_count++;
        }
    }
    
    if (sys_count == 0) {
        terminal_writestring("No system tasks.\n");
        return;
    }
    
    terminal_writestring("=== System Tasks ===\n");
    terminal_writestring("ID   Priority  Status    Progress  Title\n");
    terminal_writestring("---  --------  --------  --------  -----\n");
    
    for (int i = 0; i < MAX_TASKS; i++) {
        if (task_list[i].id == 0 || task_list[i].type != TASK_TYPE_SYSTEM) continue;
        
        // ID
        char id_str[8];
        itoa(task_list[i].id, id_str);
        terminal_writestring(id_str);
        terminal_writestring("   ");
        
        // Priority
        const char* priority_str = "";
        switch (task_list[i].priority) {
            case PRIORITY_LOW: priority_str = "Low     "; break;
            case PRIORITY_NORMAL: priority_str = "Normal  "; break;
            case PRIORITY_HIGH: priority_str = "High    "; break;
            case PRIORITY_CRITICAL: priority_str = "Critical"; break;
        }
        terminal_writestring(priority_str);
        terminal_writestring("  ");
        
        // Status
        const char* status_str = "";
        switch (task_list[i].status) {
            case TASK_PENDING: status_str = "Pending "; break;
            case TASK_RUNNING: status_str = "Running "; break;
            case TASK_COMPLETED: status_str = "Done    "; break;
            case TASK_PAUSED: status_str = "Paused  "; break;
            case TASK_FAILED: status_str = "Failed  "; break;
        }
        terminal_writestring(status_str);
        terminal_writestring("  ");
        
        // Progress bar
        terminal_writestring("[");
        int filled = task_list[i].progress / 10;
        for (int j = 0; j < 10; j++) {
            terminal_putchar(j < filled ? '=' : '-');
        }
        terminal_writestring("] ");
        
        char progress_str[4];
        itoa(task_list[i].progress, progress_str);
        terminal_writestring(progress_str);
        terminal_writestring("%  ");
        
        // Title
        terminal_writestring(task_list[i].title);
        terminal_writestring("\n");
    }
    
    terminal_writestring("---  --------  --------  --------  -----\n");
}

void task_list_user() {
    int user_count = 0;
    for (int i = 0; i < MAX_TASKS; i++) {
        if (task_list[i].id != 0 && task_list[i].type == TASK_TYPE_USER) {
            user_count++;
        }
    }
    
    if (user_count == 0) {
        terminal_writestring("No user tasks.\n");
        return;
    }
    
    terminal_writestring("=== User Tasks ===\n");
    terminal_writestring("ID   Status    Progress  Title\n");
    terminal_writestring("---  --------  --------  -----\n");
    
    for (int i = 0; i < MAX_TASKS; i++) {
        if (task_list[i].id == 0 || task_list[i].type != TASK_TYPE_USER) continue;
        
        // ID
        char id_str[8];
        itoa(task_list[i].id, id_str);
        terminal_writestring(id_str);
        terminal_writestring("   ");
        
        // Status
        const char* status_str = "";
        switch (task_list[i].status) {
            case TASK_PENDING: status_str = "Pending "; break;
            case TASK_RUNNING: status_str = "Running "; break;
            case TASK_COMPLETED: status_str = "Done    "; break;
            case TASK_PAUSED: status_str = "Paused  "; break;
            case TASK_FAILED: status_str = "Failed  "; break;
        }
        terminal_writestring(status_str);
        
        // Progress bar
        terminal_writestring("[");
        int filled = task_list[i].progress / 10;
        for (int j = 0; j < 10; j++) {
            terminal_putchar(j < filled ? '=' : '-');
        }
        terminal_writestring("] ");
        
        char progress_str[4];
        itoa(task_list[i].progress, progress_str);
        terminal_writestring(progress_str);
        terminal_writestring("%  ");
        
        // Title
        terminal_writestring(task_list[i].title);
        terminal_writestring("\n");
    }
    
    terminal_writestring("---  --------  --------  -----\n");
}
// System task implementations
void system_task_memory_cleanup() {
    terminal_writestring("[SYSTEM] Memory cleanup task executed\n");
    // In a real OS, this would free up unused memory pages
    // For now, it's just a placeholder
}

void system_task_disk_check() {
    terminal_writestring("[SYSTEM] Disk check task executed\n");
    // In a real OS, this would check disk integrity
    // For now, it's just a placeholder
}

void system_task_cache_flush() {
    terminal_writestring("[SYSTEM] Cache flush task executed\n");
    // In a real OS, this would flush all caches
    // For now, it's just a placeholder
}

void system_task_log_rotate() {
    terminal_writestring("[SYSTEM] Log rotation task executed\n");
    // In a real OS, this would rotate log files
    // For now, it's just a placeholder
}

void system_task_user_sync() {
    terminal_writestring("[SYSTEM] User sync task executed\n");
    // In a real OS, this would sync user data
    // For now, it's just a placeholder
}

int execute_system_task(uint16_t task_id) {
    task_t* task = task_get(task_id);
    if (!task || task->type != TASK_TYPE_SYSTEM) {
        return 0; // Not a system task
    }
    
    // Mark as running
    task->status = TASK_RUNNING;
    task->progress = 10;
    
    // Execute based on system task type
    switch (task->sys_task_type) {
        case SYS_TASK_MEMORY_CLEANUP:
            system_task_memory_cleanup();
            break;
        case SYS_TASK_DISK_CHECK:
            system_task_disk_check();
            break;
        case SYS_TASK_CACHE_FLUSH:
            system_task_cache_flush();
            break;
        case SYS_TASK_LOG_ROTATE:
            system_task_log_rotate();
            break;
        case SYS_TASK_USER_SYNC:
            system_task_user_sync();
            break;
        default:
            terminal_writestring("[SYSTEM] Unknown system task type\n");
            task->status = TASK_FAILED;
            return 0;
    }
    
    // Mark as completed
    task->progress = 100;
    task->status = TASK_COMPLETED;
    task->completion_time = current_time++;
    
    return 1;
}