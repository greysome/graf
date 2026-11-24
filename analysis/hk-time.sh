#!/bin/bash
set -euo pipefail

mydir="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
tmpdir='./analysis/tmp'
raw_csv='./analysis/hk-time.csv'
hopcroft_karp='./build/release/bin/hopcroft-karp'

cd "$mydir/.."
mkdir -p $tmpdir
echo 'family,nv,rep,time' > $raw_csv

run_case() {
  for rep in {0..4}; do
    local family=$1
    local nv=$2
    local input=$3

    echo $hopcroft_karp -t $input

    $hopcroft_karp -t $input > $tmpdir/out

    echo -n "$family,$nv,$rep," >> $raw_csv
    head -n1 $tmpdir/out | awk -F= '{ print $2 }' >> $raw_csv
  done
}

run_case 'rand-0.1'  1000  rand:500,500,0.1
run_case 'rand-0.1'  2000  rand:1000,1000,0.1
run_case 'rand-0.1'  4000  rand:2000,2000,0.1
run_case 'rand-0.1'  8000  rand:4000,4000,0.1
run_case 'rand-0.1'  16000 rand:8000,8000,0.1
run_case 'rand-0.001' 1000  rand:500,500,0.001
run_case 'rand-0.001' 2000  rand:1000,1000,0.001
run_case 'rand-0.001' 4000  rand:2000,2000,0.001
run_case 'rand-0.001' 8000  rand:4000,4000,0.001
run_case 'rand-0.001' 16000 rand:8000,8000,0.001
run_case 'rand-0.001' 32000 rand:16000,16000,0.001
run_case 'rand-0.001' 64000 rand:32000,32000,0.001

for nv in 1000 2000 4000 8000 16000 32000 64000 128000; do
  python ./graphs/generators/increasing-paths.py -v $nv > /dev/null
  mv ./graphs/increasing-paths-v${nv}.graf $tmpdir
  run_case 'increasing-paths' $nv $tmpdir/increasing-paths-v$nv.graf
done