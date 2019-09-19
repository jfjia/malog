#include "malog-impl.h"

namespace malog {

std::unique_ptr<Logger> logger;
std::atomic<uint16_t> threshold = { LEVEL_DEBUG };

inline bool should_log(Level level) {
    return static_cast<uint16_t>(level) <= threshold.load();
}

void open_log(const char* file_name, std::size_t buf_mb, std::size_t rotate_mb, bool overflow_block) {
    std::size_t limit_items = buf_mb * 1024 * 1024 / DATA_SIZE;
    logger.reset(new Logger(file_name, limit_items, rotate_mb, overflow_block));
}

Data* get_data(Level level) {
    if (should_log(level)) {
        return logger->get(level);
    } else {
        return nullptr;
    }
}

void set_level(Level level) {
    threshold.store(static_cast<uint16_t>(level));
}

void deliver_log(Data* data) {
    logger->put(data);
}

void log_printf(Level level, const char* fmt, ...) {
    Data* data = get_data(level);
    if (data) {
        data->level = static_cast<uint16_t>(level);
        data->ts = timestamp_now();
        va_list args;
        va_start(args, fmt);
        int len = vsnprintf(data->text, sizeof(data->text), fmt, args);
        va_end(args);
        if (len < 0) {
            data->len = 0;
        } else {
            data->len = static_cast<uint16_t>(len);
            if (data->len > sizeof(data->text)) {
                data->len = sizeof(data->text);
            }
        }
        deliver_log(data);
    }
}

} // namespace malog
