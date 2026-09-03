#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "report.h"

static char reason_letter(EndReason r)
{
    switch (r) {
        case END_FINISHED: return 'F';
        case END_HELD:     return 'H';
        case END_LOST:     return 'L';
        case END_KILLED:   return 'K';
        default:           return '?';
    }
}

int write_report(const Workload *w, const Timeline *tl, Algorithm alg)
{
    char path[64];
    FILE *f;
    int i;

    snprintf(path, sizeof path, "%s_%s.out", (alg == ALG_RATE) ? "rate" : "edf", LOGIN);

    f = fopen(path, "w");
    if (f == NULL) {
        fprintf(stderr, "scheduler: nao foi possivel criar '%s': %s\n", path, strerror(errno));
        return EXIT_IO;
    }

    fprintf(f, "EXECUTION BY %s\n\n", (alg == ALG_RATE) ? "RATE" : "EDF");

    for (i = 0; i < tl->count; i++) {
        const Block *b = &tl->blocks[i];
        if (b->task < 0) {
            fprintf(f, "idle for %d units\n", b->units);
        } else {
            fprintf(f, "[%s] for %d units - %c\n",
                    w->tasks[b->task].name, b->units, reason_letter(b->reason));
        }
    }

    fprintf(f, "\nLOST DEADLINES\n");
    for (i = 0; i < w->n; i++) {
        fprintf(f, "[%s] %d\n", w->tasks[i].name, w->tasks[i].lost);
    }

    fprintf(f, "\nCOMPLETE EXECUTION\n");
    for (i = 0; i < w->n; i++) {
        fprintf(f, "[%s] %d\n", w->tasks[i].name, w->tasks[i].completed);
    }

    fprintf(f, "\nKILLED\n");
    for (i = 0; i < w->n; i++) {
        fprintf(f, "[%s] %d\n", w->tasks[i].name, w->tasks[i].killed);
    }

    if (fclose(f) != 0) {
        fprintf(stderr, "scheduler: erro ao gravar '%s': %s\n", path, strerror(errno));
        return EXIT_IO;
    }

    return EXIT_OK;
}
