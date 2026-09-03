#include "scheduler.h"

typedef struct {
    int active, remaining, abs_deadline;
} Instance;

static int pick(const Workload *w, const Instance *in, int edf)
{
    int best = -1, i;

    for (i = 0; i < w->n; i++) {
        if (!in[i].active || in[i].remaining <= 0) continue;
        if (best < 0) {
            best = i;
        } else if (edf) {
            if (in[i].abs_deadline < in[best].abs_deadline) best = i;
        } else {
            if (w->tasks[i].period < w->tasks[best].period) best = i;
        }
    }
    return best;
}

static void emit(FILE *out, const Workload *w, int task, int units, char mark)
{
    if (units <= 0) return;
    if (task < 0)
        fprintf(out, "idle for %d units\n", units);
    else
        fprintf(out, "[%s] for %d units - %c\n", w->tasks[task].name, units, mark);
}

static void section(FILE *out, const Workload *w, const char *title, int which)
{
    int i;

    fprintf(out, "\n%s\n", title);
    for (i = 0; i < w->n; i++) {
        const Task *t = &w->tasks[i];
        fprintf(out, "[%s] %d\n", t->name,
                which == 0 ? t->lost : which == 1 ? t->completed : t->killed);
    }
}

void simulate(Workload *w, int edf, FILE *out)
{
    Instance in[MAX_TASKS] = {{0, 0, 0}};
    int t, i, cur = -2, len = 0;

    fprintf(out, "EXECUTION BY %s\n\n", edf ? "EDF" : "RATE");

    for (t = 0; t < w->total_time; t++) {
        int sel;

        for (i = 0; i < w->n; i++) {
            if (in[i].active && in[i].abs_deadline <= t) {
                in[i].active = 0;
                w->tasks[i].lost++;
                if (cur == i) {
                    emit(out, w, cur, len, 'L');
                    cur = -2;
                    len = 0;
                }
            }
        }

        for (i = 0; i < w->n; i++) {
            if (t % w->tasks[i].period == 0) {
                in[i].active = 1;
                in[i].remaining = w->tasks[i].burst;
                in[i].abs_deadline = t + w->tasks[i].deadline;
            }
        }

        sel = pick(w, in, edf);

        if (sel != cur) {
            if (cur != -2) emit(out, w, cur, len, 'H');
            cur = sel;
            len = 0;
        }

        len++;

        if (sel >= 0 && --in[sel].remaining == 0) {
            in[sel].active = 0;
            w->tasks[sel].completed++;
            emit(out, w, sel, len, 'F');
            cur = -2;
            len = 0;
        }
    }

    if (cur != -2) emit(out, w, cur, len, 'K');

    for (i = 0; i < w->n; i++)
        if (in[i].active) w->tasks[i].killed++;

    section(out, w, "LOST DEADLINES", 0);
    section(out, w, "COMPLETE EXECUTION", 1);
    section(out, w, "KILLED", 2);
}
