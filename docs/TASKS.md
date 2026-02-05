# Task Management System - Usage Guide

## Overview
The task management system has been successfully integrated into the Lakos kernel. All the logic runs in the kernel, and you can interact with it through CLI commands. The system supports both user tasks and system tasks.

## Features
- **Create user tasks** with title and description
- **Create system tasks** with predefined types
- **List all tasks** with status and progress information
- **Show detailed task information**
- **Update task status** (pending, running, completed, paused, failed)
- **Update task progress** (0-100%)
- **Delete tasks**
- **Execute system tasks** with automated operations

## Available Task Commands

### 1. List All Tasks
```bash
task
task list
```
Shows all tasks (user and system) in a table format with:
- Task ID
- Status (Pending, Running, Done, Paused, Failed)
- Progress bar (0-100%)
- Task title

### 2. Create a New User Task
```bash
task add "My Task Title" "Task description here"
```
Returns the task ID of the newly created task.

### 3. Show Task Details
```bash
task show <id>
```
Displays detailed information about a specific task:
- ID
- Title
- Description
- Type (User/System)
- Status
- Priority
- Progress percentage

### 4. Update Task Status
```bash
task status <id> <status>
```
Valid statuses:
- `pending` - Task is pending
- `running` - Task is currently running
- `completed` - Task is done
- `paused` - Task is paused
- `failed` - Task failed

Example:
```bash
task status 1 running
task status 1 completed
```

### 5. Update Task Progress
```bash
task progress <id> <percentage>
```
Update task progress (0-100). Setting to 100% automatically marks task as completed.

Example:
```bash
task progress 1 50
task progress 1 100
```

### 6. Delete a Task
```bash
task delete <id>
```
Removes a task from the system.

## System Tasks Management

### Available System Task Types
System tasks are special kernel tasks with higher priority:
- **0** - Memory Cleanup (SYS_TASK_MEMORY_CLEANUP)
- **1** - Disk Check (SYS_TASK_DISK_CHECK)
- **2** - Cache Flush (SYS_TASK_CACHE_FLUSH)
- **3** - Log Rotate (SYS_TASK_LOG_ROTATE)
- **4** - User Sync (SYS_TASK_USER_SYNC)

### Create System Task
```bash
task system create <type> "description"
```
Creates a system task of the specified type.

Example:
```bash
task system create 0 "Cleanup unused memory"
task system create 1 "Check disk integrity"
task system create 2 "Flush all caches"
```

### List System Tasks
```bash
task system list
```
Shows all system tasks with priorities and statuses.

### List User Tasks
```bash
task system user
```
Shows all user-created tasks.

### Execute System Task
```bash
task system run <id>
```
Executes a system task immediately.

Example:
```bash
task system run 1
```

### View Task Statistics
```bash
task system stats
```
Displays:
- Total number of tasks
- Number of system tasks
- Number of user tasks

## Built-in System Tasks

The following system tasks are automatically created when the kernel boots:
1. **Memory Cleanup** (Type: Memory Cleanup)
2. **Disk Check** (Type: Disk Check)
3. **Cache Flush** (Type: Cache Flush)
4. **User Sync** (Type: User Sync)

These tasks start with "pending" status and can be executed manually using the `task system run` command.

## Example Usage Session

### User Tasks
```bash
task add "Write documentation" "Write the usage guide"
# Task created with ID: 5

task add "Fix bugs" "Fix critical bugs in kernel"
# Task created with ID: 6

task list
# Shows list of all tasks

task show 5
# Shows details of task 5

task status 5 running
# Update task 5 status to running

task progress 5 75
# Update task 5 progress to 75%

task status 5 completed
# Mark task 5 as completed

task delete 6
# Delete task 6
```

### System Tasks
```bash
task system list
# Shows all system tasks

task system stats
# Show task statistics

task system run 1
# Execute system task with ID 1

task system create 0 "Custom cleanup"
# Create new system task of type Memory Cleanup

task system run 5
# Execute the newly created system task
```

## Implementation Details

### Kernel Components

#### Header File: `kernel/include/tasks.h`
Defines:
- Task structures and enums
- Task types (User, System)
- System task types enum
- Task priority levels (Low, Normal, High, Critical)
- Task status values (Pending, Running, Completed, Paused, Failed)
- Kernel API functions

#### Implementation: `kernel/tasks.c`
Provides:
- Task management in kernel memory
- Support for both user and system tasks
- Supports up to 32 concurrent tasks
- Task creation (user and system)
- Task status and progress updates
- Task deletion
- System task execution
- Automatic timestamp tracking
- Separate listing for user and system tasks

#### CLI Integration: `kernel/commands.c`
- Parses task commands and subcommands
- Handles user input validation
- Displays formatted output
- Integrates with existing shell
- Supports system task management

#### Kernel Initialization: `kernel/kernel.c`
- Initializes built-in system tasks on boot
- Creates 4 default system tasks
- Calls task initialization before shell starts

### Maximum Limits
- **Max concurrent tasks**: 32 (shared between user and system)
- **Title length**: 63 characters
- **Description length**: 255 characters

### Task Priority System
- **Low (0)**: Low priority tasks
- **Normal (1)**: Regular tasks (default for user tasks)
- **High (2)**: High priority (default for system tasks)
- **Critical (3)**: Critical priority for emergency system tasks

## Technical Architecture

The task system uses:
- Static in-memory storage (no disk persistence)
- Simple ID-based tracking
- Monotonic time counter
- Separate counters for user and system tasks
- Status and progress management
- Task type classification
- Priority levels with enforcement for system tasks

All task logic runs in kernel mode with no user-space dependencies.

## Built-in System Task Operations

When a system task is executed with `task system run`, it performs the following actions:

1. **Memory Cleanup**: Identifies and marks unused memory blocks
2. **Disk Check**: Verifies disk sector accessibility
3. **Cache Flush**: Flushes all kernel caches
4. **Log Rotate**: Rotates system log files
5. **User Sync**: Synchronizes user data to persistent storage

All operations execute with elevated kernel privileges.
