#!/bin/bash

PROBLEM_COUNT=90
CPU_COUNT=$(grep "^processor" /proc/cpuinfo | wc -l)

set -euo pipefail

cd $(dirname "$0")

make -j NDEBUG=1

# Ignore problems for which we were seeing diminishing returns.
#
# These are problems where the ratio of score changes approaches 1,
# which suggests we probably already got the best answers for them.
#
# Also ignore problems that are too hard -- these are problems that
# require deliberate placement of blockers, and our randomized search
# need a lot more luck to get there.
#
# +1.00: 23, 40, 42, 43, 55
# +0.99: 8, 24, 25, 26, 27, 41, 44, 45
# Too hard: 10, 13, 14, 15, 16
seq $PROBLEM_COUNT \
   | grep -vE '(23|40|42|43|55)$' \
   | grep -vE '(8|24|25|26|27|41|44|45)$' \
   | grep -vE '(10|13|14|15|16)$' \
   | xargs -n 1 -P $CPU_COUNT perl run.pl
