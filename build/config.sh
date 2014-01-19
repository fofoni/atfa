#!/bin/bash
CURR_DIR=`pwd`
cd "$( dirname "${BASH_SOURCE[0]}" )"
rm -rf debug release
mkdir debug release
cd debug
cmake -DCMAKE_BUILD_TYPE=Debug ../../
cd ../release
cmake -DCMAKE_BUILD_TYPE=Release ../../
cd $CURR_DIR
