#!/bin/bash

cd "$(dirname "$0")/.." || exit 1

if [ ! -x ./scheduler ]; then
    echo "scheduler nao encontrado: rode make antes"
    exit 1
fi

tmp_err=$(mktemp)
falhas=0
total=0

verifica() {
    desc=$1
    shift
    total=$((total + 1))

    rm -f rate_maf.out edf_maf.out
    saida=$("$@" 2> "$tmp_err")
    codigo=$?
    msg=$(cat "$tmp_err")
    problemas=

    [ "$codigo" -eq 0 ] && problemas="$problemas [saiu com codigo 0]"
    [ -n "$saida" ] && problemas="$problemas [escreveu em stdout]"
    [ -z "$msg" ] && problemas="$problemas [nada em stderr]"
    if [ -f rate_maf.out ] || [ -f edf_maf.out ]; then
        problemas="$problemas [criou arquivo de saida]"
    fi

    if [ -z "$problemas" ]; then
        printf 'OK  cod=%d  %s\n           %s\n' "$codigo" "$desc" "$(echo "$msg" | head -1)"
    else
        printf 'FALHA  %s:%s\n' "$desc" "$problemas"
        falhas=$((falhas + 1))
    fi
}

echo "--- argumentos de linha de comando"
verifica "sem argumentos"                ./scheduler
verifica "apenas o algoritmo"            ./scheduler rate
verifica "argumentos demais"             ./scheduler rate voo.txt extra
verifica "algoritmo desconhecido"        ./scheduler fifo voo.txt
verifica "algoritmo em maiusculas"       ./scheduler RATE voo.txt

echo
echo "--- acesso ao arquivo"
verifica "arquivo inexistente"           ./scheduler rate nao_existe.txt
verifica "diretorio no lugar do arquivo" ./scheduler rate testes

ilegivel=$(mktemp /tmp/maf_ilegivel.XXXXXX)
printf '10\nX 5 5 2\n' > "$ilegivel"
chmod 000 "$ilegivel"
verifica "arquivo ilegivel (chmod 000)"  ./scheduler rate "$ilegivel"
chmod 600 "$ilegivel"
rm -f "$ilegivel"

echo
echo "--- conteudo do arquivo"
verifica "arquivo vazio"                 ./scheduler rate testes/erro_vazio.txt
verifica "tempo total nao numerico"      ./scheduler rate testes/erro_tempo_invalido.txt
verifica "tempo total zero"              ./scheduler rate testes/erro_tempo_zero.txt
verifica "nenhuma tarefa"                ./scheduler rate testes/erro_sem_tarefas.txt
verifica "campo faltando"                ./scheduler rate testes/erro_campo_faltando.txt
verifica "campo nao numerico"            ./scheduler rate testes/erro_nao_numerico.txt
verifica "valor zero"                    ./scheduler rate testes/erro_valor_zero.txt
verifica "valor negativo"                ./scheduler rate testes/erro_negativo.txt
verifica "campos em excesso"             ./scheduler rate testes/erro_campos_excesso.txt

echo
echo "--- restricoes da especificacao"
verifica "deadline maior que periodo"    ./scheduler rate testes/erro_d_maior_p.txt
verifica "burst maior que deadline"      ./scheduler edf testes/erro_c_maior_d.txt

rm -f "$tmp_err"

echo
if [ "$falhas" -eq 0 ]; then
    echo "todos os $total casos de erro passaram"
else
    echo "$falhas de $total casos com problema"
fi

exit "$falhas"
