#!/bin/bash

BRed='\e[1;31m'
Color_Off='\e[0m'

DBGDIR="debug"
RLSDIR="release"

set -e

for build_dir in "$DBGDIR" "$RLSDIR"; do
    if [[ -e "$build_dir" ]]; then
        if [[ ! -d "$build_dir" ]]; then
            printf "Fatal: %s\n" \
                   "file ‘${build_dir}’ exists and is not an empty directory." >&2
            exit 1
        fi
        if ! rmdir "$build_dir"; then
            exit 2
        fi
    fi
    mkdir "$build_dir"
done

(
    cd -- "$DBGDIR"
    cmake -DCMAKE_BUILD_TYPE="Debug" ../../
)

(
    cd -- "$RLSDIR"
    cmake -DCMAKE_BUILD_TYPE="Release" ../../
)
