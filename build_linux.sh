cd tools
./format_code.sh
cd ..
mkdir build_linux
cd build_linux
cmake -DWITH_EXAMPLES=ON ..
make clean
make -j2 >1.log 2>2.log
cd ..
