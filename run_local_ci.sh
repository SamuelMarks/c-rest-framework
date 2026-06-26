#!/bin/bash
set -e
echo "Running Clang Build..."
mkdir -p build_clang && cd build_clang
cmake .. -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON > cmake_clang.log 2>&1
make -j4 > make_clang.log 2>&1 || (cat make_clang.log | grep -i "error:" && exit 1)
ctest --output-on-failure
cd ..
