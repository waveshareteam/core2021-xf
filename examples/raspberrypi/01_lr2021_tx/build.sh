#!/bin/bash

set -e
mkdir -p build
cd build
cmake -G "CodeBlocks - Unix Makefiles" ..
make
cd ..
size build/01_lr2021_tx
