@echo off
echo "build libyuv"

pushd .
echo "mkdir build_win32"
rmdir /S /Q build_win32
mkdir build_win32
cd build_win32

echo "cmake .."
cmake .. -G"Visual Studio 16 2019" -A win32 -T clangcl

echo "cmake --build win32"
cmake --build . --config Debug --target yuvconvert
cmake --build . --config Release --target yuvconvert
popd

mkdir lib\win32\Debug lib\win32\Release
copy /y build_win32\Debug\yuv.lib lib\win32\Debug\yuv.lib
copy /y build_win32\Release\yuv.lib lib\win32\Release\yuv.lib

pushd .
echo "mkdir build_win64"
rmdir /S /Q build_win64
mkdir build_win64
cd build_win64

echo "cmake .."
cmake .. -G"Visual Studio 16 2019" -A x64 -T clangcl

echo "cmake --build win64"
cmake --build . --config Debug --target yuvconvert
cmake --build . --config Release --target yuvconvert
popd

mkdir lib\win64\Debug lib\win64\Release
copy /y build_win64\Debug\yuv.lib lib\win64\Debug\yuv.lib
copy /y build_win64\Release\yuv.lib lib\win64\Release\yuv.lib
