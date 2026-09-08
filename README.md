# Escalonamento de tarefas críticas de voo

Simulador que compara dois escalonadores preemptivos de tempo real, **rate-monotonic** e
**earliest-deadline-first**, sobre tarefas periódicas.

Infraestrutura de Software — CESAR School — Implementação 3 — Login: `maf`

## Sistema operacional

Implementado e testado em **Ubuntu 24.04 sobre WSL2** (Windows 11), com `gcc 13.3.0` e
`GNU Make 4.3`. Usa apenas a biblioteca padrão C11, sem dependências externas.

## Arquivos

| Arquivo | Responsabilidade |
|---|---|
| `main.c` | Valida os argumentos, escolhe o algoritmo, abre o arquivo de saída e devolve o código de saída |
| `parser.c` / `parser.h` | Lê o arquivo de entrada e valida os campos, a positividade e `C <= D <= P` |
| `scheduler.c` / `scheduler.h` | Motor de simulação tick a tick, comum aos dois algoritmos, e escrita do resultado |
| `task.h` | Tipos compartilhados, limites e códigos de saída |
| `Makefile` | `make` gera o executável; `make clean` remove objetos, executável e saídas |
| `voo.txt` | Exemplo do enunciado |
| `testes/` | Entradas de teste, saídas esperadas e os dois scripts de bateria |

## Compilar

```bash
make clean
make
```

## Executar

```bash
./scheduler rate voo.txt
./scheduler edf  voo.txt
```

O resultado vai para `rate_maf.out` ou `edf_maf.out`. Nada é escrito na saída padrão; erros vão para
stderr, com código de saída 1 (argumentos), 2 (arquivo) ou 3 (conteúdo inválido).

## Testar

```bash
bash testes/rodar.sh    # 12 comparacoes: 6 entradas x 2 algoritmos
bash testes/erros.sh    # 19 casos de entrada invalida
```

Saídas esperadas: `todos os 12 casos passaram` e `todos os 19 casos de erro passaram`.

O caso de "arquivo ilegível" cria o arquivo em `/tmp` porque em `/mnt/c` o WSL não guarda permissão
POSIX e o `chmod 000` não teria efeito.

## Formato de entrada

```
[TEMPO TOTAL]
[NOME] [PERÍODO] [DEADLINE] [BURST]
```

Inteiros positivos, com `C <= D <= P`. Todas as tarefas chegam no instante 0.

## Marcadores da saída

`F` concluiu no prazo · `H` foi preemptada · `L` perdeu o deadline · `K` cortada pelo fim da simulação
