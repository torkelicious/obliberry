#pragma once
#include <string>

namespace IO::Package::Tools {
    void log_info(const std::string &binary_name, const std::string &msg);

    void log_error(const std::string &binary_name, const std::string &msg);
}
