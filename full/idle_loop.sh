#!/bin/bash

PROBLEM_COUNT=90
CPU_COUNT=$(grep "^processor" /proc/cpuinfo | wc -l)

i=0

make clean
make -j NDEBUG=1

while true; do
   i=$((i+1))
   echo "================================================================ $i"
   seq $PROBLEM_COUNT \
      | grep -vE '(23|40|42|43|55)$' \
      | grep -vE '(8|24|25|26|27|41|44|45)$' \
      | grep -vE '(10|13|14|15|16)$' \
      | shuf \
      | xargs -n 1 -P $CPU_COUNT perl run.pl
done
