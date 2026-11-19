#!/bin/bash
set -u

mydir="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
graf_info="${mydir}/../build/debug/bin/graf-info"
if [[ ! -f "$graf_info" ]]; then
    echo "Could not find debug build of graf-info (${graf_info})"
    exit 1
fi

total_tests=0
passed_tests=0

run_test() {
  file=$1
  expected=$2

  basename="$(basename "$file")"
  echo "${basename}:"
  echo -e "  expected:  ${expected}"

  actual=$("$graf_info" "$file" 2>&1)
  echo -e "  actual:    ${actual}"

  total_tests=$((total_tests+1))
  if [[ "$expected" == "$actual" ]]; then
    passed_tests=$((passed_tests+1))
  fi
}

run_test "${mydir}/graphs/valid-empty.graf"                   'no errors'
run_test "${mydir}/graphs/K4-vertex-labels.graf"              'no errors'
run_test "${mydir}/graphs/bad-header-short.graf"              'graf_open() failed (graf: bad header)'
run_test "${mydir}/graphs/bad-header-magic.graf"              'graf_open() failed (graf: bad header)'
run_test "${mydir}/graphs/body-abrupt-eof.graf"               'graf_load() failed (graf: abrupt end-of-file)'
run_test "${mydir}/graphs/unknown-segment.graf"               'graf_load() failed (graf: unknown segment)'
run_test "${mydir}/graphs/vertex-record-abrupt-eof.graf"      'graf_load() failed (graf: abrupt end-of-file)'
run_test "${mydir}/graphs/arc-label-valid.graf"               'no errors'
run_test "${mydir}/graphs/arc-label-conflict.graf"            'graf_load() failed (graf: attribute already defined)'
run_test "${mydir}/graphs/arc-label-bad-attr-index.graf"      'graf_load() failed (graf: attribute index out of bounds)'
run_test "${mydir}/graphs/arc-endpoint-oob.graf"              'graf_load() failed (graf: vertex index out of bounds)'
run_test "${mydir}/graphs/arc-record-abrupt-eof.graf"         'graf_load() failed (graf: abrupt end-of-file)'
run_test "${mydir}/graphs/arc-array-before-vertex-array.graf" 'graf_load() failed (graf: vertex array not seen before arc array)'
run_test "${mydir}/graphs/bipartite-valid.graf"               'no errors'
run_test "${mydir}/graphs/bipartite-four-vertex-valid.graf"   'no errors'
run_test "${mydir}/graphs/bipartite-same-part-arc.graf"       'graf_load() failed (graf: bad arc in bipartite segment)'
run_test "${mydir}/graphs/bipartite-invalid-part-value.graf"  'graf_load() failed (graf: bad arc in bipartite segment)'
run_test "${mydir}/graphs/bipartite-label-conflict.graf"      'graf_load() failed (graf: attribute already defined)'
run_test "${mydir}/graphs/bipartite-bad-part-sizes.graf"      'graf_load() failed (graf: bad part sizes in bipartite segment)'
run_test "${mydir}/graphs/bipartite-bad-attr-index.graf"      'graf_load() failed (graf: attribute index out of bounds)'
echo -e "\n${passed_tests}/${total_tests} tests passed"

if [[ $passed_tests -lt $total_tests ]]; then
  exit 1
fi
