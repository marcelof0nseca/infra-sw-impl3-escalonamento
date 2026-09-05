#!/bin/bash

cd "$(dirname "$0")/.." || exit 1

if [ ! -x ./scheduler ]; then
    echo "scheduler nao encontrado: rode make antes"
    exit 1
fi

falhas=0

for caso in voo uma_tarefa empate killed limite sobrecarga; do
    if [ "$caso" = voo ]; then
        entrada=voo.txt
    else
        entrada=testes/$caso.txt
    fi

    for alg in rate edf; do
        esperado=testes/esperado_${alg}_${caso}.out
        ./scheduler "$alg" "$entrada"
        if diff -q "${alg}_maf.out" "$esperado" > /dev/null; then
            printf 'OK     %-4s %s\n' "$alg" "$caso"
        else
            printf 'FALHA  %-4s %s\n' "$alg" "$caso"
            diff "$esperado" "${alg}_maf.out"
            falhas=$((falhas + 1))
        fi
    done
done

echo
if [ "$falhas" -eq 0 ]; then
    echo "todos os 12 casos passaram"
else
    echo "$falhas caso(s) com divergencia"
fi

exit "$falhas"
