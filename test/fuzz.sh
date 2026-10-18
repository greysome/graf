#!/bin/bash
set -ue

mydir="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
cd ${mydir}/..

make afl

AFL_SKIP_CPUFREQ=1 \
  afl-fuzz -i "${mydir}/graphs" -o "${mydir}/fuzz-out" \
  -- "${mydir}/../graf-info" @@

make clean