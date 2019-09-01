#include "malog.h"

int main(int argc, char** argv) {
    MALOG_OPEN_STDIO(1, 0, true);
    MALOG_INFO("Welcome");
    return 0;
}
