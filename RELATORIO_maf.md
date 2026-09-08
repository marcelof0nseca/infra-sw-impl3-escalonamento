# Escalonamento de tarefas críticas de voo

**Infraestrutura de Software — CESAR School — Implementação 3**
Login: `maf`

---

## 1. Objetivo

O enunciado pede um simulador que compare dois escalonadores preemptivos de tempo real —
rate-monotonic e earliest-deadline-first — sobre tarefas periódicas com período, deadline relativo
e rajada de CPU, gravando a linha do tempo de execução e as contagens de deadlines perdidos,
execuções completas e instâncias mortas no fim da simulação. Entrego um executável `scheduler` em
C, compilado por `make` sem alvo, que escolhe o algoritmo por argumento, grava `rate_maf.out` ou
`edf_maf.out` e trata as entradas inválidas listadas sem nunca falhar de forma não controlada.

---

## 2. Checklist de Requisitos

| Requisito do enunciado | Status | Como testar / evidência |
|---|---|---|
| `make` sem alvo produz único executável `scheduler` | OK | `make clean && make` com `-Wall -Wextra`, **zero warnings** |
| `./scheduler rate voo.txt` gera `rate_maf.out` | OK | `diff rate_maf.out testes/esperado_rate_voo.out` → **idêntico ao gabarito do enunciado** |
| `./scheduler edf voo.txt` gera `edf_maf.out` | OK | saída conferida contra simulação manual e fixada em `testes/esperado_edf_voo.out` |
| Nada é impresso em stdout na execução normal | OK | `./scheduler rate voo.txt > stdout_check.txt` e `wc -c` → `0` |
| Rate-monotonic preemptivo, prioridade por menor período | OK | preempção de NAV por ATT em t=20, reproduzida byte a byte no gabarito |
| EDF preemptivo, prioridade por menor deadline absoluto | OK | na mesma execução, o edf **não** preempta em t=20 (30 < 32) e **preempta** em t=60 (72 < 80) |
| Desempate pela ordem de aparição no arquivo | OK | `testes/empate.txt`: IMU chega com deadline igual ao de NAV e não preempta → `[NAV] for 4 units - F` |
| Perda de deadline descarta a rajada restante no instante exato | OK | `[NAV] for 2 units - L` em t=30 com 1 u.t. restante, igual ao enunciado |
| Instância que termina exatamente no deadline conta como completa | OK | `testes/limite.txt` (`C == D == P == 5`) → `COMPLETE EXECUTION 4`, `LOST DEADLINES 0` |
| Seções LOST DEADLINES / COMPLETE EXECUTION / KILLED | OK | `testes/killed.txt` → `[MAP] for 3 units - K` e `KILLED [MAP] 1` |
| Erro: número incorreto de argumentos | OK | `testes/erros.sh`, casos E1–E3, código de saída 1 |
| Erro: algoritmo diferente de rate/edf | OK | E4 (`fifo`) e E5 (`RATE` maiúsculo), código 1 |
| Erro: arquivo inexistente ou ilegível | OK | E6, E7 (diretório) e E8 (`chmod 000`), código 2 |
| Erro: arquivo malformado | OK | E9–E17, código 3, com a linha e o campo nomeados na mensagem |
| Erro: `D > P` ou `C > D` | OK | E18 e E19, código 3, citando os valores e a restrição violada |
| Erro sai com código ≠ 0 e não cria arquivo de saída | OK | as 4 condições verificadas nos 19 casos: código ≠ 0, stderr não vazio, stdout vazio, nenhum `.out` criado |
| README descrevendo os `.c`, compilação, execução e SO | OK | `README.md` na raiz do pacote |
| Makefile com compilação e limpeza | OK | `make` e `make clean` |
| Bugs plantados B2 (trava) e B3 (mente) | **Não feito** | planejados no roteiro; não executados por falta de tempo — ver seção 8 |

---

## 3. Como Reproduzir

Ambiente: **Ubuntu 24.04 sobre WSL2** (Windows 11), kernel `6.6.87.2-microsoft-standard-WSL2
x86_64`, `gcc 13.3.0`, `GNU Make 4.3`. Só biblioteca padrão C11.

Compilar:

```bash
make clean
make
```

Executar:

```bash
./scheduler rate voo.txt
./scheduler edf  voo.txt
```

Validar contra o gabarito publicado no enunciado:

```bash
diff rate_maf.out testes/esperado_rate_voo.out && echo "IDENTICO AO GABARITO"
```

Baterias completas:

```bash
bash testes/rodar.sh    # 12 comparações: 6 entradas x 2 algoritmos
bash testes/erros.sh    # 19 casos inválidos
```

Saída esperada: `todos os 12 casos passaram` e `todos os 19 casos de erro passaram`. Os dois scripts
terminam com código de saída igual ao número de falhas.

---

## 4. Arquitetura

**Arquivos e responsabilidades:**

1. `main.c` → valida os argumentos, escolhe o algoritmo, abre o arquivo de saída, devolve o código de saída
2. `task.h` → tipos compartilhados (`Task`, `Workload`), limites e códigos de erro
3. `parser.c` / `parser.h` → lê o arquivo de entrada e valida tudo: campos, positividade, `C <= D <= P`
4. `scheduler.c` / `scheduler.h` → motor de simulação de uma unidade de tempo por iteração, comum aos dois algoritmos, escrevendo o resultado

Os dois algoritmos diferem em **um único ponto**: a função `pick_task`. O rate compara o período das
tarefas; o EDF compara o deadline absoluto da instância viva. Preempção, desempate, perda de
deadline e contagem são o mesmo código.

**Decisões técnicas:**

1. **Simulação tick a tick, não fila de eventos** → o enunciado define preempção imediata e perda de
   deadline "no exato instante do deadline", que são eventos de granularidade 1 u.t. → custo
   O(T × N), irrelevante nestas escalas, e a decisão é reavaliada a cada unidade, eliminando uma
   classe inteira de erro de ordenação de eventos.

2. **Ordem fixa dentro de cada instante: expira → chega → escolhe → executa** → quando `D == P`, a
   instância antiga expira no mesmo tick em que a nova chega; se a chegada viesse primeiro, ela
   sobrescreveria a instância que precisa ser contada como perdida → é a ordem que faz
   `testes/sobrecarga.txt` produzir `LOST DEADLINES [TER] 2` em vez de 1.

3. **Um motor só, com o critério de prioridade como única diferença** → nenhuma correção pode ser
   aplicada a um algoritmo e esquecida no outro → em compensação, o rate não pode ganhar otimização
   específica sem tocar no caminho do EDF.

4. **Escrita direto no arquivo durante a simulação** → os blocos saem em ordem cronológica, então o
   vetor intermediário não comprava nada → sumiram `malloc`, a struct `Timeline`, o `timeline_free`
   e o tratamento de estouro; o projeto caiu de **558 para 334 linhas** e de 8 para 6 arquivos.
   Custo assumido: o motor passou a conhecer o formato de saída.

5. **Erros concentrados numa função variádica `fail`**, que cuida de mensagem em stderr, `fclose` e
   código de retorno → cada validação vira uma linha → 19 casos de erro cobertos sem repetição de
   `fclose` espalhada pelo parser.

6. **Três códigos de saída distintos** (1 uso, 2 acesso ao arquivo, 3 conteúdo inválido) → o
   enunciado só exige "diferente de zero", mas separar permite testar com `echo $?` sem depender do
   texto da mensagem → foi o que tornou a bateria `erros.sh` verificável de forma automática.

7. **Parser tolerante a `\r` de fim de linha** → o arquivo pode ser editado no Windows e lido no WSL
   → sem isso, um byte invisível grudado no último campo transformaria um número válido em erro de
   parsing.

---

## 5. Estratégias e Diário de Desenvolvimento

### 5.1 Estratégias

| Estratégia | Nome curto | Contexto | Motivo da troca |
|---|---|---|---|
| **S1** | Motor único parametrizado | Simular o exemplo do enunciado no papel antes de escrever código, fixar a ordem dos eventos dentro do tick e implementar um só laço, com rate e edf diferindo apenas no critério de prioridade. | Não houve troca. O rate bateu com o gabarito na primeira execução e o EDF, no mesmo motor, acertou a previsão manual — nenhum sintoma exigiu repensar a abordagem. |
| **S2** | Redução da superfície | Com a corretude já provada por `diff`, reescrever para o menor tamanho possível **sem remover nenhuma validação**: saída escrita durante a simulação, módulo `report` absorvido, `malloc` eliminado. | Não foi troca por falha, e sim decisão de simplificar **depois** de ter uma referência de corretude. A ordem importa: o `diff` contra o gabarito virou teste de regressão da reescrita. |
| **S3** | Previsão antes da execução | Para os 5 casos de teste próprios, escrever as 12 saídas esperadas à mão **antes** de rodar o programa uma única vez. | Complementar às anteriores. Salvar a saída do próprio programa como "esperado" não testa nada — congela o bug junto com o acerto. Com gabarito oficial disponível para um único caso, os outros precisavam de referência independente do código. |

### 5.2 Diário de Tentativas

| # | Estrat. | O que tentei | Resultado | Hipótese/Causa | Quando | Evidência |
|---|---|---|---|---|---|---|
| 1 | S1 | Simular o exemplo do enunciado à mão, instante a instante, antes de escrever código | OK — as 13 linhas conferiram e ficou claro que a expiração de deadline precisa vir antes das chegadas | — | 03/09, antes das 14:18 | contratos fixados no planejamento |
| 2 | S1 | Descartar e regravar o `evidencias.log`, que capturara dados pessoais de terceiro na config do git | OK | — | 03/09 13:53 → 14:18 | log começa em 14:18:21 |
| 3 | S1 | `make clean && make` com `-Wall -Wextra -std=c11 -O2` | OK — zero warnings | — | 03/09 14:21 | `evidencias.log` |
| 4 | S1 | Validar o rate com `diff` contra o exemplo do enunciado | OK — idêntico byte a byte na primeira execução | — | 03/09 14:21 | `IDENTICO AO GABARITO` |
| 5 | S1 | Registrar por escrito a previsão do EDF antes de executá-lo | OK — confirmada integralmente | — | 03/09 14:22:13 | `edf_maf.out` |
| 6 | S2 | Remover todos os comentários dos fontes, recompilando e revalidando antes de commitar | OK — `diff` idêntico | — | 03/09 ~15:00 | `evidencias.log` |
| 7 | S2 | Primeiro `git push` do projeto | **Falhou** — `! [rejected] main -> main (fetch first)` | O repositório remoto já continha um commit inicial criado pelo GitHub; o histórico local não o tinha, e aceitar o push o apagaria. Resolvido com `git pull --rebase origin main`, que reaplica os commits locais por cima e mantém o histórico linear | 03/09 ~15:40 | `git log --oneline` |
| 8 | S2 | Reescrever para o menor tamanho possível | OK — 558 → **334 linhas** (−40%), 8 → 6 arquivos, `diff` continuou idêntico | — | 03/09 15:47 e 15:56 | commit `15f317b`: 165 inserções, 389 remoções |
| 9 | S3 | Escrever à mão as 12 saídas esperadas antes de executar qualquer caso novo | OK — 12 de 12 na primeira execução, inclusive a previsão mais arriscada | — | 04/09 21:42:25 | `testes/rodar.sh` |
| 10 | S3 | Testar o caso obrigatório "arquivo ilegível" com `chmod 000` dentro do projeto | **Falhou** — o arquivo continuou legível, `cat` saiu com código 0 | `/mnt/c` é montado como DrvFs, sem metadata POSIX: o `chmod 000` foi traduzido para o atributo read-only do Windows e o `ls -l` mostrou `-r-xr-xr-x`, não `----------`. Passei a criar o arquivo em `/tmp` (ext4), onde `chmod 000` produz `Permission denied` | 05/09 20:28 | comparação dos dois `ls -l` no log |
| 11 | S3 | Corrigir a mensagem para diretório passado no lugar do arquivo | OK — passou de "arquivo vazio" para `erro ao ler 'testes': Is a directory` | — | 05/09 20:38 | caso E7 |
| 12 | S3 | Bateria de 19 casos inválidos, 4 condições cada | OK — 19 de 19 | — | 05/09 20:38:50 | `testes/erros.sh` |
| 13 | S3 | **Bug plantado B1**: remover a checagem de `fopen() == NULL` | OK como teste dirigido — segfault, código 139, `fp=0x0` no backtrace | — | 08/09 17:41 | `print_dia3_erro_principal.png` |
| 14 | S3 | Rodar a bateria de erros com o B1 plantado | OK — 2 falhas em 19, mas acusadas como `[nada em stderr]`, não pelo código de saída | O `Segmentation fault` é impresso pelo **shell**, não pelo processo. Como 139 ≠ 0, a condição do código de saída foi satisfeita: um script que só checasse o código teria aprovado o segfault | 08/09 17:43 | `evidencias.log` |
| 15 | S3 | Restaurar com `git checkout -- parser.c` e repetir o mesmo bloco de comandos | OK — código 2, mensagem correta, gdb respondendo `No stack.` | — | 08/09 17:45:57 | `print_dia3_erro_corrigido.png` |

---

## 6. Evidências

### 6.1 Log automático

`evidencias.log`, gravado com `script -a evidencias.log` em todas as sessões, incluído no `maf.tar`.
Cada sessão começa com `date; whoami; pwd` e tem `date` entre os blocos de teste, dando âncoras de
tempo que batem com o `git log`.

O log é aberto com `bind 'set enable-bracketed-paste off'`, `export PS1='$ '` e `unset LS_COLORS`.
Isso não é cosmético: na primeira sessão o terminal tinha 63 colunas e o bash reescrevia cada
comando colado, enchendo o arquivo de centenas de sequências de escape por linha.

Uma decisão de integridade: a primeira gravação capturou, na configuração do git, o e-mail de outra
pessoa. Em vez de editar o trecho, **descartei o log inteiro e regravei a sessão do zero**, antes de
qualquer teste técnico ter sido executado — log com trecho apagado é indício de adulteração.

Duas observações sobre o log, para que nada nele pareça inconsistente:

- A sessão iniciada em 05/09 às 20:27 aparece encerrada em 08/09 às 17:36, porque o terminal ficou
  aberto entre os dois dias. Ao perceber, no dia 8, que o gravador havia sido fechado **antes** do
  laboratório do bug plantado, **refiz os testes com o gravador ligado** em vez de descrever no
  relatório uma execução que o log não continha. Por isso o B1 aparece no log às 18:01–18:10, e os
  prints correspondem a essa execução gravada.
- Uma das sessões termina sem a linha `Script done`: o computador reiniciou sozinho durante a
  captura do print de validação final. A sessão seguinte recomeça alguns minutos depois e refaz a
  verificação inteira.

### 6.2 Prints

| Arquivo | O que prova |
|---|---|
| `prints/print_dia3_erro_principal.png` | Bug B1 plantado: `Segmentation fault`, `codigo de saida: 139`, e o backtrace do gdb com `_IO_fgets (..., fp=0x0)` → `parse_input at parser.c:61` → `main at main.c:31` |
| `prints/print_dia3_erro_corrigido.png` | **Mesmo bloco de comandos** após `git checkout -- parser.c`: código 2, mensagem `nao foi possivel abrir 'nao_existe.txt': No such file or directory`, e o gdb respondendo `No stack.` porque não houve crash |
| `prints/print_dia4_validacao_final.png` | Pacote extraído em pasta limpa, `make` sem warning, 12 de 12 casos válidos e 19 de 19 casos de erro |

---

## 7. Uso de IA

**Onde usei IA.** Usei o Claude (Claude Code) como par de programação ao longo de toda a
implementação: escrita do código C, do Makefile, dos scripts de teste, do README e deste relatório,
além do planejamento dos casos de teste e do cálculo à mão das saídas esperadas. A divisão foi
fixada no começo: a IA escreve e explica, eu executo todos os comandos, capturo as saídas, tiro os
prints e faço os commits. Nenhum resultado foi escrito nos documentos antes de eu ter colado a
saída real do terminal.

**Prompts principais:**
- Método de trabalho: divisão em dias por item da rubrica, arquivos de acompanhamento, disciplina de commits e de evidências.
- "Explique o mecanismo de rate-monotonic e EDF antes do código" e "simule o exemplo do enunciado à mão, instante a instante".
- "Escreva o parser com todas as validações do enunciado" / "escreva o motor de simulação tick a tick".
- "Remova os comentários dos arquivos que vão para o versionamento."
- "Simplifique o código ao máximo, deixe o menor possível."
- "Monte casos de teste que provem o desempate, o marcador K e a fronteira C == D == P, com as saídas esperadas calculadas antes de rodar."
- "Plante um bug clássico de tratamento de erro e me guie no diagnóstico até a causa raiz."

**O que validei manualmente e como.** Executei pessoalmente toda compilação e todo teste, e conferi
o `diff` contra o gabarito publicado no enunciado — nenhuma afirmação de corretude deste relatório
vem de leitura de código, todas vêm de execução registrada no `evidencias.log`. Conferi a saída do
EDF do `voo.txt` linha a linha contra a simulação manual antes de aceitá-la como referência. Rodei
o diagnóstico do `chmod` que derrubou a premissa inicial do teste de arquivo ilegível. Fiz o
diagnóstico do bug plantado com `gdb` e li o backtrace. Extraí o pacote em pasta limpa e recompilei
do zero para conferir que a entrega funciona fora do diretório de desenvolvimento.

---

## 8. Reflexão Final

A decisão que mais rendeu foi separar corretude de tamanho: primeiro fazer o rate bater byte a byte
com o gabarito, e só depois reescrever o projeto para 40% do tamanho usando aquele `diff` como teste
de regressão. Sem essa ordem, a simplificação teria sido um salto no escuro. A segunda foi escrever
as saídas esperadas à mão antes de rodar — o que transforma o teste em verificação de verdade, em
vez de fotografia do comportamento atual. O erro que mais me ensinou não foi de código: foi supor
que `chmod 000` funcionaria no diretório do projeto, quando `/mnt/c` é DrvFs e não guarda permissão
POSIX — um caso obrigatório do enunciado era impossível de testar ali. Deixei de fazer os bugs
plantados B2 e B3 por falta de tempo, e o B3 era o mais valioso: comparar deadline relativo em vez
de absoluto transforma o EDF em Deadline-Monotonic disfarçado, produz saída plausível e passaria em
parte dos casos. Na próxima, planto os bugs no mesmo dia em que a feature fica pronta, em vez de
deixar para o fim — cheguei a deixar um bug plantado no diretório de trabalho por três dias, o que
só não virou problema porque a regra de commitar antes de plantar manteve o repositório limpo.

---

## 9. Checklist Final de Entrega

| Item | Confirmado? |
|---|---|
| Compilei do zero seguindo só o que está no relatório | Sim — `make clean && make`, e também a partir do pacote extraído em `/tmp` |
| Rodei os testes e `evidencias.log` foi gerado | Sim — 12 casos válidos e 19 de erro, log incluído no `.tar` |
| Tenho prints obrigatórios | Sim — erro principal, erro corrigido e validação final |
| Testei pelo menos um caso limite e um caso inválido | Sim — `limite.txt` (`C == D == P`) e 19 casos inválidos |
| Preenchi a seção de uso de IA | Sim |
| Revisei o relatório e removi frases genéricas | Sim |

---

## 10. Se eu tivesse mais 2 horas

Executaria os bugs plantados B2 e B3, que ficaram de fora: o B3 em especial, porque medir **quais**
casos de teste continuam passando com o EDF trocado por Deadline-Monotonic quantifica o valor de ter
cinco casos em vez de um. Rodaria a suíte inteira sob `valgrind` — o programa não tem mais `malloc`
depois da simplificação, mas a leitura do arquivo ainda tem caminhos que nunca foram checados por
ferramenta. Acrescentaria um teste com muitas tarefas para exercitar o limite de `MAX_TASKS`, hoje
validado mas sem caso dedicado. Trocaria o vetor fixo de tarefas por alocação dinâmica, eliminando o
limite arbitrário de 64. Faria um caso com nomes de tarefa repetidos, que hoje o parser aceita sem
reclamar e que produziria seções de estatística ambíguas. E mediria o tempo de execução com
horizontes grandes (`T` na casa dos milhões) para saber a partir de que ponto o custo O(T × N) do
laço tick a tick deixa de ser irrelevante — hoje afirmo que é irrelevante nas escalas do enunciado,
mas não medi.
