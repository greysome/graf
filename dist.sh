#!/bin/bash
set -eu pipefail

mkdir -p dist/{include,lib,bin}
cp README.md dist/
cp LICENSE dist/
cp graf.h dist/include/
cp build/release/libgraf.a dist/lib/
cp build/release/graf-info dist/bin/
tar -czf graf-$(uname -m).tar.gz -C dist .