#!/bin/bash
set -ue

mydir="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
graf_info="${mydir}/../build/afl/graf-info"
if [[ ! -f "$graf_info" ]]; then
    echo "Could not find AFL-instrumented build of graf-info (${graf_info})"
    exit 1
fi
cd ${mydir}/..

make afl

AFL_SKIP_CPUFREQ=1 \
  afl-fuzz -i "${mydir}/graphs" -o "${mydir}/fuzz-out" \
  -- "$graf_info" @@

make clean