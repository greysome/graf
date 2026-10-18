#!/bin/bash
set -u

mydir="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
total_tests=0
passed_tests=0

run_test() {
  file=$1
  expected=$2

  basename="$(basename "$file")"
  echo "${basename}:"
  echo -e "  expected:  ${expected}"

  actual=$("${mydir}/../graf-info" "$file" 2>&1)
  echo -e "  actual:    ${actual}"

  total_tests=$((total_tests+1))
  if [[ "$expected" == "$actual" ]]; then
    passed_tests=$((passed_tests+1))
  fi
}

run_test "${mydir}/graphs/valid-empty.graf" 'ok!'
run_test "${mydir}/graphs/K4-vertex-labels.graf" 'ok!'
run_test "${mydir}/graphs/bad-header-short.graf" 'bad header'
run_test "${mydir}/graphs/bad-header-magic.graf" 'bad header'
run_test "${mydir}/graphs/body-abrupt-eof.graf" 'abrupt end-of-file'
run_test "${mydir}/graphs/unknown-segment.graf" 'unknown segment'
run_test "${mydir}/graphs/vertex-record-abrupt-eof.graf" 'abrupt end-of-file'
run_test "${mydir}/graphs/arc-endpoint-oob.graf" 'vertex index out of bounds'
run_test "${mydir}/graphs/arc-record-abrupt-eof.graf" 'abrupt end-of-file'
run_test "${mydir}/graphs/arc-array-before-vertex-array.graf" 'vertex array not seen before arc array'
run_test "${mydir}/graphs/bipartite-valid.graf" 'ok!'
run_test "${mydir}/graphs/bipartite-four-vertex-valid.graf" 'ok!'
run_test "${mydir}/graphs/bipartite-same-part-arc.graf" 'bad arc in bipartite segment'
run_test "${mydir}/graphs/bipartite-invalid-part-value.graf" 'bad arc in bipartite segment'
run_test "${mydir}/graphs/bipartite-label-conflict.graf" 'attribute already defined'
run_test "${mydir}/graphs/bipartite-bad-part-sizes.graf" 'bad part sizes in bipartite segment'
run_test "${mydir}/graphs/bipartite-bad-attr-index.graf" 'attribute index out of bounds'
echo -e "\n${passed_tests}/${total_tests} tests passed"
