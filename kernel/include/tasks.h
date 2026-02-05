#ifndef TASKS_H
#define TASKS_H

#include <stdint.h>

// Task priority levels
typedef enum {
    PRIORITY_LOW = 0,
    PRIORITY_NORMAL = 1,
    PRIORITY_HIGH = 2,
    PRIORITY_CRITICAL = 3
} task_priority_t;

// Task status
typedef enum {
    TASK_PENDING = 0,
    TASK_RUNNING = 1,
    TASK_COMPLETED = 2,
    TASK_PAUSED = 3,
    TASK_FAILED = 4
} task_status_t;

// Task type
typedef enum {
    TASK_TYPE_USER = 0,
    TASK_TYPE_SYSTEM = 1
} task_type_t;

// System task types
typedef enum {
    SYS_TASK_MEMORY_CLEANUP = 0,
    SYS_TASK_DISK_CHECK = 1,
    SYS_TASK_CACHE_FLUSH = 2,
    SYS_TASK_LOG_ROTATE = 3,
    SYS_TASK_USER_SYNC = 4,
    SYS_TASK_CUSTOM = 5
} system_task_type_t;

// Task structure
typedef struct {
    uint16_t id;
    char title[64];
    char description[256];
    task_status_t status;
    task_priority_t priority;
    task_type_t type;
    uint32_t created_time;
    uint32_t due_time;
    uint32_t completion_time;
    uint8_t progress; // 0-100
    uint16_t sys_task_type; // For system tasks
} task_t;

// Task management functions
void tasks_init();
uint16_t task_create(const char* title, const char* description, task_priority_t priority, uint32_t due_time);
uint16_t task_create_system(const char* title, const char* description, system_task_type_t sys_type, task_priority_t priority);
int task_update_status(uint16_t task_id, task_status_t status);
int task_delete(uint16_t task_id);
int task_set_progress(uint16_t task_id, uint8_t progress);
void task_list_all();
void task_list_system();
void task_list_user();
void task_show_details(uint16_t task_id);
int task_count();
int task_count_system();
int task_count_user();
task_t* task_get(uint16_t task_id);

// System task operations
void system_task_memory_cleanup();
void system_task_disk_check();
void system_task_cache_flush();
void system_task_log_rotate();
void system_task_user_sync();
int execute_system_task(uint16_t task_id);

#endif
