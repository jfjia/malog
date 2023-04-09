#ifndef MALOG_IMPL_H
#define MALOG_IMPL_H

#include <chrono>
#include <atomic>
#include <memory>
#include <cstdarg>
#include <cstdio>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <string>
#include <cstring>
#include <vector>
#include <sys/stat.h>
#include <iostream>
#include "malog.h"

namespace malog {

// formatted line:
// [2019-09-20 14:33:48.313] [INFO ] Hello logger: msg number 0[EOL]
// where [EOL] is "\r\n" for Windows and "\n" for Linux

static const int ts_tag_len1 = 21; // "[2019-09-20 14:33:48."
static const int ts_tag_len2 = 5;  // "313] "
static const int ts_tag_len = ts_tag_len1 + ts_tag_len2;

static const int level_tag_len = 8;
static const char* level_tag[] = {
  "[OFF  ] ",
  "[ERROR] ",
  "[WARN ] ",
  "[INFO ] ",
  "[DEBUG] "
};

#ifndef _WIN32
static const int line_max_len = ts_tag_len + level_tag_len + DATA_TEXT_SIZE + 1;
#else
static const int line_max_len = ts_tag_len + level_tag_len + DATA_TEXT_SIZE + 2;
#endif

inline uint64_t timestamp_now() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

class DataPool {
public:
  DataPool(std::size_t limit_items, bool overflow_block);

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
      while (true) {
        pool_cond_.wait(l);
        if (pool_.size() > 0) {
          Data* data = pool_.back();
          pool_.pop_back();
          return data;
        } else {
          continue;
        }
      }
    }
  }

  void put(Data* data) {
    std::lock_guard<std::mutex> l(pool_lock_);
    pool_.push_back(data);
    pool_cond_.notify_one();
  }

  void put(std::vector<Data* >& data) {
    std::lock_guard<std::mutex> l(pool_lock_);
    for (auto i : data) {
      pool_.push_back(i);
    }
    pool_cond_.notify_all();
  }

protected:
  std::mutex pool_lock_;
  std::vector<Data* > pool_;
  std::condition_variable pool_cond_;
  bool overflow_block_;
  Data* head_{ nullptr };
};

DataPool::DataPool(std::size_t limit_items, bool overflow_block)
  : overflow_block_(overflow_block) {
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
  void write(std::vector<Data* >& items);

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
  std::time_t t_cached_{ 0 };
  char cached_str_[line_max_len];
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

void FileWriter::write(std::vector<Data* >& items) {
  for (auto data : items) {
    std::time_t t = data->ts / 1000;
    if (t != t_cached_) {
      auto tm = std::localtime(&t);
      sprintf(cached_str_, "[%04d-%02d-%02d %02d:%02d:%02d.",
              tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
              tm->tm_hour, tm->tm_min, tm->tm_sec);
      t_cached_ = t;
    }
    // time stamp
    int len = ts_tag_len;
    sprintf(cached_str_ + ts_tag_len1, "%03d] ", (int)(data->ts % 1000));
    // level
    memcpy(cached_str_ + len, level_tag[data->level], level_tag_len);
    len += level_tag_len;
    // text
    memcpy(cached_str_ + len, data->text, data->len);
    len += data->len;
#ifndef _WIN32
    // EOL
    cached_str_[len] = '\n';
    len ++;
#else
    // EOL
    cached_str_[len] = '\r';
    len ++;
    cached_str_[len] = '\n';
    len ++;
#endif
    len = fwrite(cached_str_, len, 1, fp_);
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
}

void FileWriter::close() {
  if (fp_ && fp_ != stdout) {
    fclose(fp_);
  }
  fp_ = nullptr;
}

void FileWriter::rotate() {
  close();

  std::time_t time_t = timestamp_now() / 1000;
  auto tm = std::localtime(&time_t);
  char newpath[2048];
  snprintf(newpath, 2048, "%s.%04d%02d%02d-%02d%02d%02d.%03d",
           file_name_.c_str(),
           tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
           tm->tm_hour, tm->tm_min, tm->tm_sec,
           (int)(time_t % 1000));
  if (::rename(file_name_.c_str(), newpath) == -1) {
    std::cerr << "cannot rename: " << file_name_ << " to " << newpath << std::endl;
  }

  open();
  current_size_ = 0;
}

class Logger {
public:
  Logger(const std::string& file_name, std::size_t limit_items, std::size_t rotate_mb, bool overflow_block)
    : limit_items_(limit_items), pool_(limit_items, overflow_block), writer_(file_name, rotate_mb) {
    items_.reserve(limit_items);
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

  void consume(std::vector<Data* >& data) {
    writer_.write(data);
    pool_.put(data);
    data.clear();
  }

protected:
  bool shut_{ false };
  std::mutex write_lock_;
  std::condition_variable write_cond_;
  std::vector<Data* > items_;
  std::thread write_thread_;
  std::size_t limit_items_;
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
  items.reserve(limit_items_);
  while (true) {
    {
      std::unique_lock<std::mutex> l(write_lock_);
      if (shut_) {
        break;
      }
      if (items_.empty()) {
        writer_.flush();
        write_cond_.wait(l);
        continue;
      }
      std::swap(items, items_);
    }
    consume(items);
  }
  consume(items_);

  writer_.close();
}

} // namespace malog

#endif
