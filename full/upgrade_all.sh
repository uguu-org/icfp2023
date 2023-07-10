#!/bin/bash

PROBLEM_COUNT=90
CPU_COUNT=$(grep "^processor" /proc/cpuinfo | wc -l)
UPGRADE_OUTPUT_DIR=upgrade

set -euo pipefail

cd $(dirname "$0")
mkdir -p $UPGRADE_OUTPUT_DIR

make -j NDEBUG=1

seq $PROBLEM_COUNT | xargs -P $CPU_COUNT -I{} ./solve problems/{}.json $UPGRADE_OUTPUT_DIR/{}.json $UPGRADE_OUTPUT_DIR/{}.svg solutions/{}.json
