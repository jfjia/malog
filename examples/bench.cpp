#include <thread>
#include <chrono>
#include <vector>
#include <iostream>
#include "malog.h"

void bench(int howmany, int thread_n) {
    std::vector<std::thread> threads;
    int msgs_per_thread = howmany / thread_n;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < thread_n; i++) {
        threads.emplace_back([](int num, int id) {
            for (int j = 0; j < num; j++) {
                malog_info("Hello logger: msg number %d", j);
            }
        }, msgs_per_thread, i);
    }
    for (auto& i : threads) {
        i.join();
    }
    auto delta = std::chrono::high_resolution_clock::now() - start;
    auto delta_d = std::chrono::duration_cast<std::chrono::duration<double>>(delta).count();
    std::cout << "Elapsed: " << delta_d << " secs\t " << int(howmany / delta_d) << "/sec" << std::endl;;
}

void bench_stream(int howmany, int thread_n) {
    std::vector<std::thread> threads;
    int msgs_per_thread = howmany / thread_n;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < thread_n; i++) {
        threads.emplace_back([](int num, int id) {
            for (int j = 0; j < num; j++) {
                MALOG_INFO("Hello logger: msg number " << j);
            }
        }, msgs_per_thread, i);
    }
    for (auto& i : threads) {
        i.join();
    }
    auto delta = std::chrono::high_resolution_clock::now() - start;
    auto delta_d = std::chrono::duration_cast<std::chrono::duration<double>>(delta).count();
    std::cout << "Elapsed(stream): " << delta_d << " secs\t " << int(howmany / delta_d) << "/sec" << std::endl;;
}

int main(int argc, char** argv) {
    MALOG_OPEN("logs.txt", 8, 512, true);
    bench(1000000, 8);
    bench_stream(1000000, 8);
    return 0;
}
