#!/bin/sh

git submodule update --init --recursive

cmake -S Logless -B Logless/build -DCMAKE_BUILD_TYPE=Release
cmake --build Logless/build -j"$(nproc)"
