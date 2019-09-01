#ifndef MALOG_IMPL_H
#define MALOG_IMPL_H

#include <atomic>
#include <memory>
#include <cstdarg>
#include <cstdio>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <iostream>
#include "malog.h"

namespace malog {

static const int level_tag_len = 8;

static const char* level_tag[] = {
    "[OFF  ] ",
    "[ERROR] ",
    "[WARN ] ",
    "[INFO ] ",
    "[DEBUG] "
};

class DataPool {
public:
    DataPool(std::size_t limit_mb, bool overflow_block);

    void reclaim();

    Data* get(Level level) {
        std::unique_lock<std::mutex> l(pool_lock_);
        if (pool_.size() > 0) {
            Data* data = pool_.back();
            pool_.pop_back();
            return data;
        }
        if (!overflow_block_) {
            return nullptr;
        } else {
            pool_cond_.wait(l);
            if (pool_.size() > 0) {
                Data* data = pool_.back();
                pool_.pop_back();
                return data;
            } else {
                return nullptr;
            }
        }
    }

    void put(Data* data) {
        std::lock_guard<std::mutex> l(pool_lock_);
        pool_.push_back(data);
        pool_cond_.notify_all();
    }

protected:
    std::mutex pool_lock_;
    std::vector<Data* > pool_;
    std::condition_variable pool_cond_;
    bool overflow_block_;
    Data* head_{ nullptr };
};

DataPool::DataPool(std::size_t limit_mb, bool overflow_block)
    : overflow_block_(overflow_block) {
    std::size_t limit_items = limit_mb * 1024 * 1024 / DATA_SIZE;
    head_ = new Data[limit_items];
    Data* next = head_;
    for (std::size_t i = 0; i < limit_items; i++) {
        pool_.push_back(next);
        next ++;
    }
}

void DataPool::reclaim() {
    pool_.clear();
    delete head_;
}

class FileWriter {
public:
    FileWriter(const std::string& file_name, std::size_t rotate_mb)
        : file_name_(file_name), rotate_size_(rotate_mb * 1024 * 1024) {
    }

    void open();
    void write(Data* data);

    void flush() {
        fflush(fp_);
    }

    void close();

protected:
    void rotate();

protected:
    std::string file_name_;
    std::size_t rotate_size_;
    FILE* fp_{ nullptr };
    std::size_t current_size_{ 0 };
};

void FileWriter::open() {
    if (file_name_ != "@stdout") {
        fp_ = fopen(file_name_.c_str(), "ab");
        if (fp_) {
            int fd = fileno(fp_);
            struct stat st;
            if (::fstat(fd, &st) == 0) {
                current_size_ = static_cast<size_t>(st.st_size);
            } else {
                current_size_ = 0;
            }
        }
    }
    if (!fp_) {
        fp_ = stdout;
        current_size_ = 0;
        rotate_size_ = 0;
    }
}

void FileWriter::write(Data* data) {
    std::time_t time_t = data->ts / 1000000;
    auto tm = std::localtime(&time_t);
    int len = fprintf(fp_, "[%04d-%02d-%02d %02d:%02d:%02d.%06d] %.*s%.*s\n",
                      tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                      tm->tm_hour, tm->tm_min, tm->tm_sec, (int)(data->ts % 1000000),
                      level_tag_len, level_tag[data->level], (int)data->len, data->text);
    if (len > 0) {
        current_size_ += len;
    }
    if (data->level == LEVEL_ERROR) {
        fflush(fp_);
    }
    if (rotate_size_ && current_size_ >= rotate_size_) {
        rotate();
    }
}

void FileWriter::close() {
    if (fp_ && fp_ != stdout) {
        fclose(fp_);
    }
    fp_ = nullptr;
}

void FileWriter::rotate() {
    close();

    std::time_t time_t = timestamp_now() / 1000000;
    auto tm = std::localtime(&time_t);
    char newpath[2048];
    snprintf(newpath, 2048, "%s.%04d%02d%02d-%02d%02d%02d.%06d",
             file_name_.c_str(),
             tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
             tm->tm_hour, tm->tm_min, tm->tm_sec,
             (int)(time_t % 1000000));
    if (::rename(file_name_.c_str(), newpath) == -1) {
        std::cerr << "cannot rename: " << file_name_ << " to " << newpath << std::endl;
    }

    open();
    current_size_ = 0;
}

class Logger {
public:
    Logger(const std::string& file_name, std::size_t limit_mb, std::size_t rotate_mb, bool overflow_block)
        : pool_(limit_mb, overflow_block), writer_(file_name, rotate_mb) {
        write_thread_ = std::thread(&Logger::run, this);
    }

    ~Logger();

    Data* get(Level level) {
        return pool_.get(level);
    }

    void put(Data* data) {
        std::lock_guard<std::mutex> l(write_lock_);
        items_.push_back(data);
        write_cond_.notify_all();
    }

protected:
    void run();
    void consume(std::vector<Data* >& items);

protected:
    bool shut_{ false };
    std::mutex write_lock_;
    std::condition_variable write_cond_;
    std::vector<Data* > items_;
    std::thread write_thread_;
    DataPool pool_;
    FileWriter writer_;
};

Logger::~Logger() {
    {
        std::lock_guard<std::mutex> l(write_lock_);
        shut_ = true;
        write_cond_.notify_all();
    }
    write_thread_.join();
    pool_.reclaim();
}

void Logger::run() {
    writer_.open();

    std::vector<Data* > items;
    while (true) {
        {
            std::unique_lock<std::mutex> l(write_lock_);
            if (shut_) {
                break;
            }
            if (items_.empty()) {
                writer_.flush();
                write_cond_.wait(l);
            }
            std::swap(items, items_);
        }
        consume(items);
    }
    consume(items_);

    writer_.close();
}

void Logger::consume(std::vector<Data* >& items) {
    for (auto i = items.begin(); i != items.end(); i++) {
        writer_.write((*i));
        pool_.put((*i));
    }
    items.clear();
}

} // namespace malog

#endif
