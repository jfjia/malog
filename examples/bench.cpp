#include <thread>
#include <chrono>
#include <vector>
#include <iostream>
#include "malog.h"

void printf_log(int num, int id) {
  for (int j = 0; j < num; j++) {
    malog_info("Hello logger_f: msg number %d, id=%d", j, id);
  }
}

void stream_log(int num, int id) {
  for (int j = 0; j < num; j++) {
    MALOG_INFO("Hello logger_s: msg number " << j << ", id=" << id);
  }
}

void bench(int howmany, int thread_n, void(*fn)(int, int)) {
  std::vector<std::thread> threads;
  int msgs_per_thread = howmany / thread_n;
  auto start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < thread_n; i++) {
    threads.emplace_back(fn, msgs_per_thread, i);
  }
  for (auto& i : threads) {
    i.join();
  }
  auto delta = std::chrono::high_resolution_clock::now() - start;
  auto delta_d = std::chrono::duration_cast<std::chrono::duration<double>>(delta).count();
  std::cout << "Elapsed: " << delta_d << " secs\t " << int(howmany / delta_d) << "/sec" << std::endl;;
}

int main(int argc, char** argv) {
  MALOG_OPEN("logs.txt", 2, 512, true);
  std::cout << "--printf style" << std::endl;
  bench(1000000, 10, printf_log);
  std::cout << "--stream style" << std::endl;
  bench(1000000, 10, stream_log);
  return 0;
}
