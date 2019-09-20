# malog
My Async Log library use c++11

Simple(~500 lines code) and fast.

Benchmark use Lenovo Ideapad (3632QM, 16G RAM, 128G SSD), MSYS2 gcc 9.1.0

10 threads, 2MB queue, run 3 times, write to logs.txt

$ ./build_mingw/examples/bench.exe
--printf style
Elapsed: 0.561032 secs   1782429/sec
--stream style
Elapsed: 0.584033 secs   1712230/sec

$ ./build_mingw/examples/bench.exe
--printf style
Elapsed: 0.589633 secs   1695970/sec
--stream style
Elapsed: 0.576033 secs   1736011/sec

$ ./build_mingw/examples/bench.exe
--printf style
Elapsed: 0.462225 secs   2163449/sec
--stream style
Elapsed: 0.497028 secs   2011957/sec
