#ifndef TASK_H
#define TASK_H

#define MAX_TASKS 64
#define MAX_NAME  64

#define LOGIN "maf"

#define EXIT_OK       0
#define EXIT_USAGE    1
#define EXIT_IO       2
#define EXIT_FORMAT   3
#define EXIT_INTERNAL 4

typedef enum {
    ALG_RATE,
    ALG_EDF
} Algorithm;

typedef enum {
    END_FINISHED,
    END_HELD,
    END_LOST,
    END_KILLED,
    END_IDLE
} EndReason;

typedef struct {
    char name[MAX_NAME];
    int  period;
    int  deadline;
    int  burst;

    int lost;
    int completed;
    int killed;
} Task;

typedef struct {
    Task tasks[MAX_TASKS];
    int  n;
    int  total_time;
} Workload;

#endif
