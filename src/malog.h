#ifndef MALOG_H
#define MALOG_H

#undef MALOG_NO_STREAM // define MALOG_NO_STREAM to disable stream-style API

#include <cstdint>
#ifndef MALOG_NO_STREAM
#include <sstream>
#endif

namespace malog {

static const int DATA_SIZE = 256;

struct Data {
  uint16_t level;
  uint16_t len;
  uint32_t reserved;
  uint64_t ts;
  char text[0]; // formatted text
};

static const int DATA_TEXT_SIZE = (DATA_SIZE - sizeof(Data));

enum Level {
  LEVEL_OFF = 0, // discard all
  LEVEL_ERROR,
  LEVEL_WARN,
  LEVEL_INFO,
  LEVEL_DEBUG
};

void open_log(const char* file_name, // file name or "@stdout"
              std::size_t fifo_mb,   // fifo buffer size
              std::size_t rotate_mb,  // old log file rename to file_name.YYYYMMDD-HHMMSS.xxxxxx
              bool overflow_block);  // 'false' - drop, 'true' - block producer

void set_level(Level level);

void log_printf(Level level, const char* fmt, ...);

void shutdown();

#ifndef MALOG_NO_STREAM
Data* get_data(Level level);

void deliver_log(Data* data);

class LineBuffer : public std::streambuf {
public:
  LineBuffer(Data* data) {
    setp(data->text, data->text + DATA_TEXT_SIZE);
  }

  uint16_t text_len() {
    return (uint16_t)(pptr() - pbase());
  }

protected:
  virtual std::streambuf::int_type overflow(int_type c = 0) {
    // ignore any text longer than limitation
    return 0;
  }
};

class Line {
public:
  Line(Data* data, uint16_t level)
    : data_(data), buffer_(data), os_(&buffer_) {
    data->level = level;
  }

  ~Line() {
    data_->len = buffer_.text_len();
    deliver_log(data_);
  }

  std::ostream& os() {
    return os_;
  }

protected:
  Data* data_;
  LineBuffer buffer_;
  std::ostream os_;
};
#endif

} // namespace malog

#define MALOG_OPEN(NAME, BUFMB, ROTATEMB, POLICY) malog::open_log(NAME, BUFMB, ROTATEMB, POLICY)
#define MALOG_OPEN_STDIO(BUFMB, POLICY) malog::open_log("@stdout", BUFMB, 0, POLICY)
#define MALOG_SET_LEVEL(LEVEL) malog::set_level(LEVEL)

#ifndef MALOG_NO_STREAM
#define MALOG(LEVEL, logs) \
  do { \
    malog::Data* data__ = malog::get_data(LEVEL); \
    if (data__) { \
      malog::Line(data__, LEVEL).os() << logs; \
    } \
  } while (0)
#define MALOG_ERROR(logs) MALOG(malog::LEVEL_ERROR, logs)
#define MALOG_WARN(logs)  MALOG(malog::LEVEL_WARN,  logs)
#define MALOG_INFO(logs)  MALOG(malog::LEVEL_INFO,  logs)
#if defined(NDEBUG)
#define MALOG_DEBUG(logs) do {} while (0)
#else
#define MALOG_DEBUG(logs) MALOG(malog::LEVEL_DEBUG, logs)
#endif
#endif

#define malog_error(...)  malog::log_printf(malog::LEVEL_ERROR, __VA_ARGS__)
#define malog_warn(...)   malog::log_printf(malog::LEVEL_WARN,  __VA_ARGS__)
#define malog_info(...)   malog::log_printf(malog::LEVEL_INFO,  __VA_ARGS__)
#if defined(NDEBUG)
#define malog_debug(...)  do {} while(0)
#else
#define malog_debug(...)  malog::log_printf(malog::LEVEL_DEBUG, __VA_ARGS__)
#endif

#endif
