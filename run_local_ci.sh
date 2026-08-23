#!/bin/bash
set -e
echo "Running Clang Build..."
mkdir -p build_clang && cd build_clang
cmake .. -DFETCHCONTENT_SOURCE_DIR_C-ABSTRACT-HTTP="/Users/samuel/repos/c-abstract-http" -DFETCHCONTENT_SOURCE_DIR_C-ORM="/Users/samuel/repos/c-orm" -DFETCHCONTENT_SOURCE_DIR_C-STR-SPAN="/Users/samuel/repos/c-str-span" -DFETCHCONTENT_SOURCE_DIR_CDD-C="/Users/samuel/repos/cdd-c" -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON > cmake_clang.log 2>&1
make -j4 > make_clang.log 2>&1 || (cat make_clang.log | grep -i "error:" && exit 1)
ctest --output-on-failure
cd ..
