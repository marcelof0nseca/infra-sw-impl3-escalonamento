#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdio.h>
#include "task.h"

void simulate(Workload *w, int edf, FILE *out);

#endif
