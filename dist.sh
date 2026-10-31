#!/bin/bash
set -eu pipefail

rm -rf dist/*
mkdir -p dist/{include,lib,bin}
cp README.md dist/
cp LICENSE dist/
cp graf.h dist/include/
cp build/release/libgraf.a dist/lib/
cp -R build/release/bin/ dist/

os=$(uname -s | tr '[:upper:]' '[:lower:]')
arch=$(uname -m)
tar -czf "graf-${os}-${arch}.tar.gz" \
    --transform='s,^dist,graf,' \
    dist/