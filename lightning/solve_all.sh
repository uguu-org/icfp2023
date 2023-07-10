#!/bin/bash

PROBLEM_COUNT=55
CPU_COUNT=$(grep "^processor" /proc/cpuinfo | wc -l)

set -euo pipefail

cd $(dirname "$0")

make -j NDEBUG=1

seq $PROBLEM_COUNT | xargs -n 1 -P $CPU_COUNT perl run.pl
