#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#include "parser.h"

#define MAX_LINE 512

static int fail(FILE *f, int code, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    fputs("scheduler: ", stderr);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    if (f) fclose(f);
    return code;
}

static char *trim(char *s)
{
    size_t n = strlen(s);

    while (n > 0 && (s[n-1] == '\n' || s[n-1] == '\r' || s[n-1] == ' ' || s[n-1] == '\t'))
        s[--n] = '\0';
    while (*s == ' ' || *s == '\t')
        s++;
    return s;
}

static int number(const char *tok, int *out)
{
    char *end;
    long v;

    errno = 0;
    v = strtol(tok, &end, 10);
    if (end == tok || *end != '\0' || errno == ERANGE || v <= 0 || v > INT_MAX)
        return 0;

    *out = (int) v;
    return 1;
}

int parse_input(const char *path, Workload *w)
{
    static const char *campo[3] = { "PERIODO", "DEADLINE", "BURST" };
    char line[MAX_LINE], *s, *tok;
    int no = 0;
    FILE *f = fopen(path, "r");

    if (!f)
        return fail(NULL, EXIT_IO, "nao foi possivel abrir '%s': %s\n", path, strerror(errno));

    w->n = 0;
    w->total_time = 0;

    do {
        if (!fgets(line, sizeof line, f))
            return fail(f, EXIT_FORMAT, "arquivo '%s' vazio: falta o tempo total de simulacao\n", path);
        no++;
        s = trim(line);
    } while (*s == '\0');

    tok = strtok(s, " \t");
    if (!number(tok, &w->total_time))
        return fail(f, EXIT_FORMAT, "linha %d: tempo total invalido ('%s'): esperado inteiro positivo\n", no, tok);
    if (strtok(NULL, " \t"))
        return fail(f, EXIT_FORMAT, "linha %d: a primeira linha deve conter apenas o tempo total\n", no);

    while (fgets(line, sizeof line, f)) {
        int v[3], i;
        char *name;
        Task *t;

        no++;
        s = trim(line);
        if (*s == '\0') continue;

        if (w->n == MAX_TASKS)
            return fail(f, EXIT_FORMAT, "linha %d: numero de tarefas excede o maximo de %d\n", no, MAX_TASKS);

        name = strtok(s, " \t");
        if (strlen(name) >= MAX_NAME)
            return fail(f, EXIT_FORMAT, "linha %d: nome de tarefa com mais de %d caracteres\n", no, MAX_NAME - 1);

        for (i = 0; i < 3; i++) {
            tok = strtok(NULL, " \t");
            if (!tok)
                return fail(f, EXIT_FORMAT, "linha %d: campo %s ausente (esperado: NOME PERIODO DEADLINE BURST)\n", no, campo[i]);
            if (!number(tok, &v[i]))
                return fail(f, EXIT_FORMAT, "linha %d: campo %s ('%s') nao e um inteiro positivo\n", no, campo[i], tok);
        }
        if (strtok(NULL, " \t"))
            return fail(f, EXIT_FORMAT, "linha %d: campos em excesso (esperado: NOME PERIODO DEADLINE BURST)\n", no);

        if (v[1] > v[0])
            return fail(f, EXIT_FORMAT, "linha %d: tarefa '%s' tem DEADLINE %d maior que PERIODO %d, violando D <= P\n", no, name, v[1], v[0]);
        if (v[2] > v[1])
            return fail(f, EXIT_FORMAT, "linha %d: tarefa '%s' tem BURST %d maior que DEADLINE %d, violando C <= D\n", no, name, v[2], v[1]);

        t = &w->tasks[w->n++];
        snprintf(t->name, MAX_NAME, "%s", name);
        t->period = v[0];
        t->deadline = v[1];
        t->burst = v[2];
        t->lost = t->completed = t->killed = 0;
    }

    if (ferror(f))
        return fail(f, EXIT_IO, "erro ao ler '%s': %s\n", path, strerror(errno));
    fclose(f);

    if (w->n == 0)
        return fail(NULL, EXIT_FORMAT, "arquivo '%s' nao contem nenhuma tarefa\n", path);

    return EXIT_OK;
}
