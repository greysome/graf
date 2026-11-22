#!/bin/bash
set -eu

mydir="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
cd "$mydir"

total_tests=0
passed_tests=0
actual_out=""
actual_err=""

run_program() {
  local program=$1
  local opts=$2
  local return_actual=$3

  programpath="${mydir}/../build/debug/bin/${program}"
  # Convert absolute paths into relative paths from test/
  opts="${opts//${mydir}\//}"

  echo "exec: $program $opts"

  set +e
  ASAN_OPTIONS=check_printf=0 "$programpath" $opts > "${mydir}/outputs/${total_tests}out" 2> "${mydir}/outputs/${total_tests}err"
  set -e

  if [[ "$return_actual" == 1 ]]; then
    actual_out="$(< outputs/${total_tests}out)"
    actual_err="$(< outputs/${total_tests}err)"
  fi
}

test_string() {
  local program=$1
  local opts=$2
  local expected_out=$3
  local expected_err=$4

  run_program "$program" "$opts" 1
  echo -e "  expected stdout:  $expected_out"
  echo -e "  actual stdout:    $actual_out"
  echo -e "  expected stderr:  $expected_err"
  echo -e "  actual stderr:    $actual_err"
  if [[ "$expected_out $expected_err" == "$actual_out $actual_err" ]]; then
    passed_tests=$((passed_tests+1))
  fi
  total_tests=$((total_tests+1))
}

test_reference() {
  local program=$1
  local opts=$2
  local reference_out="${3//${mydir}\//}"
  local reference_err="${4//${mydir}\//}"

  actual_out_path="outputs/${total_tests}out"
  actual_err_path="outputs/${total_tests}err"

  run_program "$program" "$opts" 0

  local matches=0
  if cmp -s "$reference_out" "$actual_out_path"; then
    matches=$((matches+1))
    echo -e "  actual stdout ($actual_out_path) == reference stdout ($reference_out)"
  else
    echo -e "  actual stdout ($actual_out_path) != reference stdout ($reference_out)"
  fi

  if cmp -s "$reference_err" "$actual_err_path"; then
    matches=$((matches+1))
    echo -e "  actual stderr ($actual_err_path) == reference stderr ($reference_err)"
  else
    echo -e "  actual stderr ($actual_err_path) != reference stderr ($reference_err)"
  fi

  if [[ $matches == 2 ]]; then
    passed_tests=$((passed_tests+1))
  fi
  total_tests=$((total_tests+1))
}

cd "$mydir"
mkdir -p outputs
test_string graf-info /nonexistent-file                         '' 'graf_open() failed (graf: open() failed, OS: No such file or directory)'
test_string graf-info graphs/valid-empty.graf                   '' 'no errors'
test_string graf-info graphs/K4-vertex-labels.graf              '' 'no errors'
test_string graf-info graphs/bad-header-short.graf              '' 'graf_open() failed (graf: bad header)'
test_string graf-info graphs/bad-header-magic.graf              '' 'graf_open() failed (graf: not a .graf file)'
test_string graf-info graphs/body-abrupt-eof.graf               '' 'graf_load() failed (graf: abrupt end-of-file)'
test_string graf-info graphs/unknown-segment.graf               '' 'graf_load() failed (graf: unknown segment)'
test_string graf-info graphs/vertex-record-abrupt-eof.graf      '' 'graf_load() failed (graf: abrupt end-of-file)'
test_string graf-info graphs/arc-label-valid.graf               '' 'no errors'
test_string graf-info graphs/arc-label-conflict.graf            '' 'graf_load() failed (graf: attribute already defined)'
test_string graf-info graphs/arc-label-bad-attr-index.graf      '' 'graf_load() failed (graf: attribute index out of bounds)'
test_string graf-info graphs/arc-endpoint-oob.graf              '' 'graf_load() failed (graf: vertex index out of bounds)'
test_string graf-info graphs/arc-record-abrupt-eof.graf         '' 'graf_load() failed (graf: abrupt end-of-file)'
test_string graf-info graphs/arc-array-before-vertex-array.graf '' 'graf_load() failed (graf: vertex array not seen before arc array)'
test_string graf-info graphs/bipartite-valid.graf               '' 'no errors'
test_string graf-info graphs/bipartite-four-vertex-valid.graf   '' 'no errors'
test_string graf-info graphs/bipartite-same-part-arc.graf       '' 'graf_load() failed (graf: bad arc in bipartite segment)'
test_string graf-info graphs/bipartite-invalid-part-value.graf  '' 'graf_load() failed (graf: bad arc in bipartite segment)'
test_string graf-info graphs/bipartite-label-conflict.graf      '' 'graf_load() failed (graf: attribute already defined)'
test_string graf-info graphs/bipartite-bad-part-sizes.graf      '' 'graf_load() failed (graf: bad part sizes in bipartite segment)'
test_string graf-info graphs/bipartite-bad-attr-index.graf      '' 'graf_load() failed (graf: attribute index out of bounds)'
test_reference hopcroft-karp graphs/tchoukaillon-16.graf      references/hk-tchoukaillon.out       references/hk-tchoukaillon.err
test_reference hopcroft-karp '-t graphs/tchoukaillon-16.graf' references/hk-tchoukaillon-table.out references/hk-tchoukaillon.err
echo -e "\n${passed_tests}/${total_tests} tests passed"

if [[ $passed_tests -lt $total_tests ]]; then
  exit 1
fi
