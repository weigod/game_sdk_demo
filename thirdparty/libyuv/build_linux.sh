#!/bin/bash
echo "build libyuv"

echo "mkdir build_linux"
rm -rf build_linux 
mkdir build_linux
cd build_linux

echo "cmake .."
cmake .. -DCMAKE_C_FLAGS="-fPIC" -DCMAKE_CXX_FLAGS="-fPIC" -DCMAKE_CXX_VISIBILITY_PRESET=hidden -DCMAKE_VISIBILITY_INLINES_HIDDEN=1

echo "cmake --build ."
cmake --build .
cd -

mkdir -p lib/linux
cp build_linux/libyuv.a lib/linux/libyuv.a

