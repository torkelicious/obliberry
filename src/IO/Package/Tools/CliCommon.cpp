#include "CliCommon.h"
#include <iostream>

namespace IO::Package::Tools {
    void log_info(const std::string &binary_name, const std::string &msg) {
        std::cout << "[" << binary_name << "] " << msg << "\n";
    }

    void log_error(const std::string &binary_name, const std::string &msg) {
        std::cerr << "[" << binary_name << "] Error: " << msg << "\n";
    }
}
