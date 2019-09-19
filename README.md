# malog
My Async Log library use c++11

Benchmark use Thinkpad X280 (8G/500G SSD)

8 threads, 8MB queue

$ ./build_mingw/examples/bench.exe
Elapsed: 2.14283 secs    466672/sec
Elapsed(stream): 2.35265 secs    425052/sec

$ ./build_mingw/examples/bench.exe
Elapsed: 2.05805 secs    485897/sec
Elapsed(stream): 2.20474 secs    453568/sec

$ ./build_mingw/examples/bench.exe
Elapsed: 2.15129 secs    464837/sec
Elapsed(stream): 2.23582 secs    447262/sec
