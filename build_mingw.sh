cd tools
./format_code.sh
cd ..
mkdir build_mingw
cd build_mingw
cmake -DWITH_EXAMPLES=ON -DCMAKE_BUILD_TYPE=Release -G "MSYS Makefiles" ..
make clean
make -j2 >1.log 2>2.log
cd ..
