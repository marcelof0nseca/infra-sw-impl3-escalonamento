#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "task.h"

typedef struct {
    int       task;
    int       units;
    EndReason reason;
} Block;

typedef struct {
    Block *blocks;
    int    count;
    int    capacity;
} Timeline;

int  simulate(Workload *w, Algorithm alg, Timeline *tl);
void timeline_free(Timeline *tl);

#endif
