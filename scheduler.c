#include <stdio.h>
#include <stdlib.h>

#include "scheduler.h"

typedef struct {
    int active;
    int remaining;
    int abs_deadline;
} Instance;

static int timeline_push(Timeline *tl, int task, int units, EndReason reason)
{
    if (units <= 0) return EXIT_OK;
    if (tl->count >= tl->capacity) return EXIT_INTERNAL;

    tl->blocks[tl->count].task   = task;
    tl->blocks[tl->count].units  = units;
    tl->blocks[tl->count].reason = reason;
    tl->count++;
    return EXIT_OK;
}

static int pick_task(const Workload *w, const Instance *inst, Algorithm alg)
{
    int best = -1;
    int i;

    for (i = 0; i < w->n; i++) {
        if (!inst[i].active || inst[i].remaining <= 0) continue;

        if (best < 0) {
            best = i;
            continue;
        }

        if (alg == ALG_RATE) {
            if (w->tasks[i].period < w->tasks[best].period) best = i;
        } else {
            if (inst[i].abs_deadline < inst[best].abs_deadline) best = i;
        }
    }

    return best;
}

int simulate(Workload *w, Algorithm alg, Timeline *tl)
{
    Instance inst[MAX_TASKS];
    int t, i;
    int cur = -2;
    int len = 0;

    tl->capacity = w->total_time + 1;
    tl->count    = 0;
    tl->blocks   = malloc((size_t) tl->capacity * sizeof(Block));
    if (tl->blocks == NULL) {
        fprintf(stderr, "scheduler: falha ao alocar memoria para a linha do tempo\n");
        return EXIT_INTERNAL;
    }

    for (i = 0; i < w->n; i++) {
        inst[i].active       = 0;
        inst[i].remaining    = 0;
        inst[i].abs_deadline = 0;
    }

    for (t = 0; t < w->total_time; t++) {
        int sel;

        for (i = 0; i < w->n; i++) {
            if (inst[i].active && inst[i].abs_deadline <= t) {
                inst[i].active = 0;
                w->tasks[i].lost++;

                if (cur == i) {
                    if (timeline_push(tl, cur, len, END_LOST) != EXIT_OK) goto overflow;
                    cur = -2;
                    len = 0;
                }
            }
        }

        for (i = 0; i < w->n; i++) {
            if (t % w->tasks[i].period == 0) {
                inst[i].active       = 1;
                inst[i].remaining    = w->tasks[i].burst;
                inst[i].abs_deadline = t + w->tasks[i].deadline;
            }
        }

        sel = pick_task(w, inst, alg);

        if (sel != cur) {
            if (cur != -2) {
                EndReason r = (cur == -1) ? END_IDLE : END_HELD;
                if (timeline_push(tl, cur, len, r) != EXIT_OK) goto overflow;
            }
            cur = sel;
            len = 0;
        }

        len++;

        if (sel >= 0) {
            inst[sel].remaining--;
            if (inst[sel].remaining == 0) {
                inst[sel].active = 0;
                w->tasks[sel].completed++;
                if (timeline_push(tl, sel, len, END_FINISHED) != EXIT_OK) goto overflow;
                cur = -2;
                len = 0;
            }
        }
    }

    if (cur != -2) {
        EndReason r = (cur == -1) ? END_IDLE : END_KILLED;
        if (timeline_push(tl, cur, len, r) != EXIT_OK) goto overflow;
    }

    for (i = 0; i < w->n; i++) {
        if (inst[i].active) w->tasks[i].killed++;
    }

    return EXIT_OK;

overflow:
    fprintf(stderr, "scheduler: erro interno: linha do tempo excedeu a capacidade\n");
    timeline_free(tl);
    return EXIT_INTERNAL;
}

void timeline_free(Timeline *tl)
{
    free(tl->blocks);
    tl->blocks   = NULL;
    tl->count    = 0;
    tl->capacity = 0;
}
