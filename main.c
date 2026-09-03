#include <stdio.h>
#include <string.h>

#include "task.h"
#include "parser.h"
#include "scheduler.h"
#include "report.h"

static void usage(const char *prog)
{
    fprintf(stderr, "uso: %s <rate|edf> <arquivo_de_entrada>\n", prog);
    fprintf(stderr, "exemplo: %s rate voo.txt\n", prog);
}

int main(int argc, char *argv[])
{
    Algorithm alg;
    Workload  w;
    Timeline  tl;
    int rc;

    if (argc != 3) {
        fprintf(stderr, "scheduler: numero incorreto de argumentos (recebidos %d, esperados 2)\n",
                argc - 1);
        usage(argv[0]);
        return EXIT_USAGE;
    }

    if (strcmp(argv[1], "rate") == 0) {
        alg = ALG_RATE;
    } else if (strcmp(argv[1], "edf") == 0) {
        alg = ALG_EDF;
    } else {
        fprintf(stderr, "scheduler: algoritmo desconhecido '%s': use 'rate' ou 'edf'\n", argv[1]);
        usage(argv[0]);
        return EXIT_USAGE;
    }

    rc = parse_input(argv[2], &w);
    if (rc != EXIT_OK) return rc;

    rc = simulate(&w, alg, &tl);
    if (rc != EXIT_OK) return rc;

    rc = write_report(&w, &tl, alg);
    timeline_free(&tl);

    return rc;
}
