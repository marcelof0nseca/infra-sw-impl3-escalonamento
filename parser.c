#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#include "parser.h"

#define MAX_LINE 512

static void chomp(char *s)
{
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) {
        s[--n] = '\0';
    }
}

static int is_blank(const char *s)
{
    while (*s) {
        if (*s != ' ' && *s != '\t') return 0;
        s++;
    }
    return 1;
}

static int parse_positive(const char *tok, int *out)
{
    char *end;
    long v;

    errno = 0;
    v = strtol(tok, &end, 10);

    if (end == tok || *end != '\0') return 0;
    if (errno == ERANGE)            return 0;
    if (v <= 0 || v > INT_MAX)      return 0;

    *out = (int) v;
    return 1;
}

int parse_input(const char *path, Workload *w)
{
    FILE *f;
    char line[MAX_LINE];
    int line_no = 0;

    f = fopen(path, "r");
    if (f == NULL) {
        fprintf(stderr, "scheduler: nao foi possivel abrir '%s': %s\n",
                path, strerror(errno));
        return EXIT_IO;
    }

    w->n = 0;
    w->total_time = 0;

    line[0] = '\0';
    while (fgets(line, sizeof line, f) != NULL) {
        line_no++;
        chomp(line);
        if (!is_blank(line)) break;
        line[0] = '\0';
    }

    if (line[0] == '\0' || is_blank(line)) {
        fprintf(stderr, "scheduler: arquivo '%s' vazio: falta o tempo total de simulacao\n", path);
        fclose(f);
        return EXIT_FORMAT;
    }

    {
        char *tok = strtok(line, " \t");
        if (tok == NULL || !parse_positive(tok, &w->total_time)) {
            fprintf(stderr,
                    "scheduler: linha %d: tempo total de simulacao invalido ('%s'): "
                    "esperado um inteiro positivo\n",
                    line_no, tok ? tok : "");
            fclose(f);
            return EXIT_FORMAT;
        }
        if (strtok(NULL, " \t") != NULL) {
            fprintf(stderr,
                    "scheduler: linha %d: a primeira linha deve conter apenas o tempo total\n",
                    line_no);
            fclose(f);
            return EXIT_FORMAT;
        }
    }

    while (fgets(line, sizeof line, f) != NULL) {
        char *name, *tok;
        int period, deadline, burst;

        line_no++;
        chomp(line);
        if (is_blank(line)) continue;

        if (w->n >= MAX_TASKS) {
            fprintf(stderr, "scheduler: linha %d: numero de tarefas excede o maximo de %d\n",
                    line_no, MAX_TASKS);
            fclose(f);
            return EXIT_FORMAT;
        }

        name = strtok(line, " \t");
        if (name == NULL) continue;

        if (strlen(name) >= MAX_NAME) {
            fprintf(stderr, "scheduler: linha %d: nome de tarefa com mais de %d caracteres\n",
                    line_no, MAX_NAME - 1);
            fclose(f);
            return EXIT_FORMAT;
        }

        {
            const char *labels[3] = { "PERIODO", "DEADLINE", "BURST" };
            int values[3];
            int i;

            for (i = 0; i < 3; i++) {
                tok = strtok(NULL, " \t");
                if (tok == NULL) {
                    fprintf(stderr,
                            "scheduler: linha %d: campo %s ausente "
                            "(esperado: NOME PERIODO DEADLINE BURST)\n",
                            line_no, labels[i]);
                    fclose(f);
                    return EXIT_FORMAT;
                }
                if (!parse_positive(tok, &values[i])) {
                    fprintf(stderr,
                            "scheduler: linha %d: campo %s ('%s') nao e um inteiro positivo\n",
                            line_no, labels[i], tok);
                    fclose(f);
                    return EXIT_FORMAT;
                }
            }

            if (strtok(NULL, " \t") != NULL) {
                fprintf(stderr,
                        "scheduler: linha %d: campos em excesso "
                        "(esperado: NOME PERIODO DEADLINE BURST)\n",
                        line_no);
                fclose(f);
                return EXIT_FORMAT;
            }

            period   = values[0];
            deadline = values[1];
            burst    = values[2];
        }

        if (deadline > period) {
            fprintf(stderr,
                    "scheduler: linha %d: tarefa '%s' tem DEADLINE (%d) maior que PERIODO (%d), "
                    "violando D <= P\n",
                    line_no, name, deadline, period);
            fclose(f);
            return EXIT_FORMAT;
        }
        if (burst > deadline) {
            fprintf(stderr,
                    "scheduler: linha %d: tarefa '%s' tem BURST (%d) maior que DEADLINE (%d), "
                    "violando C <= D\n",
                    line_no, name, burst, deadline);
            fclose(f);
            return EXIT_FORMAT;
        }

        {
            Task *t = &w->tasks[w->n];
            strncpy(t->name, name, MAX_NAME - 1);
            t->name[MAX_NAME - 1] = '\0';
            t->period    = period;
            t->deadline  = deadline;
            t->burst     = burst;
            t->lost      = 0;
            t->completed = 0;
            t->killed    = 0;
            w->n++;
        }
    }

    if (ferror(f)) {
        fprintf(stderr, "scheduler: erro ao ler '%s': %s\n", path, strerror(errno));
        fclose(f);
        return EXIT_IO;
    }

    fclose(f);

    if (w->n == 0) {
        fprintf(stderr, "scheduler: arquivo '%s' nao contem nenhuma tarefa\n", path);
        return EXIT_FORMAT;
    }

    return EXIT_OK;
}
