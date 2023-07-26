@echo off
echo "build libyuv"

pushd .
echo "mkdir build_win32_mt"
rmdir /S /Q build_win32_mt
mkdir build_win32_mt
cd build_win32_mt

echo "cmake .."
cmake .. -G"Visual Studio 16 2019" -DBUILD_MT=ON -A win32 -T clangcl

echo "cmake --build win32"
cmake --build . --config Debug --target yuvconvert
cmake --build . --config Release --target yuvconvert
popd

mkdir lib\win32\Debug lib\win32\Release
copy /y build_win32_mt\Debug\yuv.lib lib\win32\Debug\yuvmtd.lib
copy /y build_win32_mt\Release\yuv.lib lib\win32\Release\yuvmt.lib

pushd .
echo "mkdir build_win64_mt"
rmdir /S /Q build_win64_mt
mkdir build_win64_mt
cd build_win64_mt

echo "cmake .."
cmake .. -G"Visual Studio 16 2019" -DBUILD_MT=ON -A x64 -T clangcl

echo "cmake --build win64"
cmake --build . --config Debug --target yuvconvert
cmake --build . --config Release --target yuvconvert
popd

mkdir lib\win64\Debug lib\win64\Release
copy /y build_win64_mt\Debug\yuv.lib lib\win64\Debug\yuvmtd.lib
copy /y build_win64_mt\Release\yuv.lib lib\win64\Release\yuvmt.lib
