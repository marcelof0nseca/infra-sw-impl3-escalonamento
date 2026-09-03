#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "task.h"
#include "parser.h"
#include "scheduler.h"

int main(int argc, char *argv[])
{
    char path[64];
    Workload w;
    FILE *out;
    int edf, rc;

    if (argc != 3) {
        fprintf(stderr, "scheduler: numero incorreto de argumentos (recebidos %d, esperados 2)\n", argc - 1);
        fprintf(stderr, "uso: %s <rate|edf> <arquivo_de_entrada>\n", argv[0]);
        return EXIT_USAGE;
    }

    if (strcmp(argv[1], "rate") == 0) {
        edf = 0;
    } else if (strcmp(argv[1], "edf") == 0) {
        edf = 1;
    } else {
        fprintf(stderr, "scheduler: algoritmo desconhecido '%s': use 'rate' ou 'edf'\n", argv[1]);
        return EXIT_USAGE;
    }

    rc = parse_input(argv[2], &w);
    if (rc != EXIT_OK) return rc;

    snprintf(path, sizeof path, "%s_%s.out", edf ? "edf" : "rate", LOGIN);
    out = fopen(path, "w");
    if (!out) {
        fprintf(stderr, "scheduler: nao foi possivel criar '%s': %s\n", path, strerror(errno));
        return EXIT_IO;
    }

    simulate(&w, edf, out);

    if (fclose(out) != 0) {
        fprintf(stderr, "scheduler: erro ao gravar '%s': %s\n", path, strerror(errno));
        return EXIT_IO;
    }

    return EXIT_OK;
}
