#ifndef TASK_H
#define TASK_H

#define MAX_TASKS 64
#define MAX_NAME  64
#define LOGIN     "maf"

#define EXIT_OK     0
#define EXIT_USAGE  1
#define EXIT_IO     2
#define EXIT_FORMAT 3

typedef struct {
    char name[MAX_NAME];
    int  period, deadline, burst;
    int  lost, completed, killed;
} Task;

typedef struct {
    Task tasks[MAX_TASKS];
    int  n, total_time;
} Workload;

#endif
