# Escalonamento de tarefas críticas de voo

Simulador que compara dois escalonadores preemptivos de tempo real — **rate-monotonic** e
**earliest-deadline-first** — sobre um conjunto de tarefas periódicas com período, deadline
relativo e tempo de CPU.

Infraestrutura de Software — CESAR School — Implementação 3
Login: `maf`

## Sistema operacional

Implementado e testado em **Ubuntu 24.04 sobre WSL2** (Windows 11), kernel
`6.6.87.2-microsoft-standard-WSL2 x86_64`, com `gcc 13.3.0` e `GNU Make 4.3`.
O código usa apenas a biblioteca padrão C (C11), sem dependências externas, e compila em
qualquer Linux com `gcc` e `make`.

## Arquivos

### Código

| Arquivo | Responsabilidade |
|---|---|
| `main.c` | Valida os argumentos de linha de comando, decide o algoritmo, abre o arquivo de saída e devolve o código de saída do programa. |
| `parser.c` / `parser.h` | Lê o arquivo de entrada e valida tudo: campos presentes, valores inteiros e positivos, e as restrições `D <= P` e `C <= D`. Nenhuma tarefa entra na simulação sem passar por aqui. |
| `scheduler.c` / `scheduler.h` | Motor de simulação, uma unidade de tempo por iteração, comum aos dois algoritmos. Escreve a linha do tempo e as três seções de estatística diretamente no arquivo de saída. |
| `task.h` | Tipos compartilhados (`Task`, `Workload`), limites do programa e códigos de saída. |
| `Makefile` | `make` sem alvo gera o executável `scheduler`; `make clean` remove objetos, executável e saídas geradas. |

O único ponto em que os dois algoritmos diferem é a função `pick_task` do `scheduler.c`: o
rate-monotonic compara o **período** das tarefas, o EDF compara o **deadline absoluto** da instância
viva. Todo o resto — preempção, desempate, perda de deadline, contagem — é o mesmo código.

### Testes

| Arquivo | Conteúdo |
|---|---|
| `voo.txt` | Exemplo publicado no enunciado. |
| `testes/uma_tarefa.txt` | Uma tarefa só: sem concorrência, os dois algoritmos coincidem. |
| `testes/empate.txt` | Duas tarefas que empatam em prioridade: testa o desempate pela ordem do arquivo. |
| `testes/killed.txt` | Rajada maior que o horizonte de simulação: testa o marcador `K` e a seção KILLED. |
| `testes/limite.txt` | `C == D == P`: a tarefa termina exatamente no instante do deadline. |
| `testes/sobrecarga.txt` | Utilização 1,2: produz `F`, `H`, `L` e `K` na mesma execução. |
| `testes/erro_*.txt` | Onze entradas inválidas (campo faltando, valor não numérico, valor não positivo, `D > P`, `C > D`, arquivo vazio, etc.). |
| `testes/esperado_*.out` | Saídas esperadas, calculadas à mão antes da primeira execução. |
| `testes/rodar.sh` | Roda os 6 casos válidos nos 2 algoritmos e compara com `diff`. |
| `testes/erros.sh` | Roda 19 casos inválidos e verifica código de saída, stderr, stdout e ausência de arquivo de saída. |

## Compilar

```bash
make clean
make
```

Compila com `-Wall -Wextra -std=c11 -O2` e produz um único executável, `scheduler`.

## Executar

```bash
./scheduler rate voo.txt
./scheduler edf  voo.txt
```

O resultado é gravado em `rate_maf.out` ou `edf_maf.out`. **Nada é escrito na saída padrão durante a
execução normal**; mensagens de erro vão para stderr.

### Formato de entrada

```
[TEMPO TOTAL]
[NOME] [PERÍODO] [DEADLINE] [BURST]
```

Uma tarefa por linha, todos os valores inteiros positivos, com `C <= D <= P`. Todas as tarefas
chegam pela primeira vez no instante 0. Linhas em branco são ignoradas.

### Formato de saída

Uma linha por bloco contíguo de execução, seguida das três seções de estatística:

| Marcador | Significado |
|---|---|
| `F` | A instância concluiu a rajada dentro do prazo. |
| `H` | Foi interrompida por uma tarefa de prioridade maior e retoma depois. |
| `L` | O deadline absoluto venceu durante a execução; a rajada restante é descartada. |
| `K` | A simulação terminou com a instância ainda executando. |

`LOST DEADLINES` conta as instâncias cujo deadline venceu dentro da simulação; `COMPLETE EXECUTION`,
as que terminaram no prazo; `KILLED`, as que ainda estavam vivas quando o tempo acabou.

### Códigos de saída

| Código | Significado |
|---|---|
| 0 | Sucesso. |
| 1 | Argumentos de linha de comando inválidos. |
| 2 | Arquivo de entrada inexistente, ilegível ou falha de escrita. |
| 3 | Arquivo malformado ou valores que violam a especificação. |

Em qualquer caso de erro o programa escreve a mensagem em stderr e **não cria arquivo de saída**.

## Testar

```bash
make
bash testes/rodar.sh    # 12 comparações: 6 entradas x 2 algoritmos
bash testes/erros.sh    # 19 casos inválidos
```

Os dois scripts terminam com código de saída igual ao número de falhas, então servem em
verificação automática. A saída esperada é `todos os 12 casos passaram` e
`todos os 19 casos de erro passaram`.

Para conferir apenas o exemplo do enunciado contra o gabarito publicado:

```bash
./scheduler rate voo.txt
diff rate_maf.out testes/esperado_rate_voo.out
```

### Observação sobre `chmod` no WSL

O caso de teste "arquivo ilegível" cria o arquivo em `/tmp`, e não no diretório do projeto, de
propósito. Em `/mnt/c` o WSL usa DrvFs, que não tem metadata de permissão POSIX: `chmod 000` é
traduzido para o atributo *read-only* do Windows, o `ls -l` mostra `-r-xr-xr-x` e o arquivo continua
legível. Em `/tmp` (ext4) o `chmod 000` funciona de verdade.
