#ifndef OBLIBERRY_STDMODULES_H
#define OBLIBERRY_STDMODULES_H

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <numbers>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include "Scripting/Interpreter/Interpreter.h"
#include "Scripting/StdLib/StdLib.h"

namespace ObSL {
    // CONVERSION LIBRARY
    class ConversionLib : public Lib {
    public:
        void register_modules(Interpreter &interpreter) override {
            // convert a number to a string
            interpreter.define_native("to_string", [](const double val) -> std::string {
                return std::to_string(val);
            });

            // convert a string to number
            interpreter.define_native("to_num", [](const std::string &str) -> double {
                try {
                    size_t idx;
                    const double val = std::stod(str, &idx);
                    if (idx != str.length()) {
                        throw std::runtime_error("Conversion Error: String contains non-numeric characters.");
                    }
                    return val;
                } catch (const std::invalid_argument &) {
                    throw std::runtime_error("Conversion Error: Cannot convert string '" + str + "' to a number.");
                } catch (const std::out_of_range &) {
                    throw std::runtime_error("Conversion Error: Number in string is out of range.");
                }
            });
        }
    };

    // MATH LIBRARY
    class MathLib : public Lib {
    public:
        void register_modules(Interpreter &interpreter) override {
            // Square Root
            interpreter.define_native("sqrt", [](const double x) -> double {
                if (x < 0.0) {
                    throw std::runtime_error("Math Error: Cannot calculate square root of a negative number.");
                }
                return std::sqrt(x);
            });

            // base^exponent
            interpreter.define_native("pow", [](const double base, const double exponent) -> double {
                return std::pow(base, exponent);
            });

            // rand num (returns a double between 0.0 and 1.0)
            interpreter.define_native("random", []() -> double {
                static std::mt19937 gen(std::random_device{}());
                static std::uniform_real_distribution dis(0.0, 1.0);
                return dis(gen);
            });

            // min/max
            interpreter.define_native("min", [](const double a, const double b) -> double { return std::min(a, b); });
            interpreter.define_native("max", [](const double a, const double b) -> double { return std::max(a, b); });

            // clamp num between min and max
            interpreter.define_native("clamp", [](const double value, const double min, const double max) -> double {
                if (min > max) throw std::runtime_error("Math Error: clamp() min cannot be greater than max.");
                return std::clamp(value, min, max);
            });

            // rounding utilities
            interpreter.define_native("floor", [](const double x) -> double { return std::floor(x); });
            interpreter.define_native("ceil", [](const double x) -> double { return std::ceil(x); });
            interpreter.define_native("round", [](const double x) -> double { return std::round(x); });
            interpreter.define_native("abs", [](const double x) -> double { return std::abs(x); });

            // trigonometry (radians)
            interpreter.define_native("sin", [](const double x) -> double { return std::sin(x); });
            interpreter.define_native("cos", [](const double x) -> double { return std::cos(x); });
            interpreter.define_native("tan", [](const double x) -> double { return std::tan(x); });
            interpreter.define_native(
                "atan2", [](const double y, const double x) -> double { return std::atan2(y, x); });

            // get pi
            interpreter.define_native("pi", []() -> double { return std::numbers::pi; });

            // degree/rad conversions
            interpreter.define_native(
                "rad", [](const double deg) -> double { return deg * (std::numbers::pi / 180.0); });
            interpreter.define_native(
                "deg", [](const double rad) -> double { return rad * (180.0 / std::numbers::pi); });

            // smoothly interpolate between start and end
            interpreter.define_native("lerp", [](const double start, const double end, const double t) -> double {
                return std::lerp(start, end, t);
            });

            // map value from an input range to an output range
            interpreter.define_native("map_value",
                                      [](const double val, const double in_min, const double in_max,
                                         const double out_min,
                                         const double out_max) -> double {
                                          if (std::abs(in_max - in_min) < 1e-9) return out_min;
                                          return out_min + (val - in_min) * (out_max - out_min) / (in_max - in_min);
                                      });
        }
    };

    // STRING LIBRARY
    class StringLib : public Lib {
    public:
        void register_modules(Interpreter &interpreter) override {
            // string length
            interpreter.define_native("len", [](const std::string &str) -> double {
                return static_cast<double>(str.length());
            });

            // convert to lowercase
            interpreter.define_native("to_lower", [](std::string str) -> std::string {
                std::ranges::transform(str, str.begin(), [](const unsigned char c) {
                    return std::tolower(c);
                });
                return str;
            });

            // convert to uppercase
            interpreter.define_native("to_upper", [](std::string str) -> std::string {
                std::ranges::transform(str, str.begin(), [](const unsigned char c) {
                    return std::toupper(c);
                });
                return str;
            });

            // check if a string starts with a specific prefix
            interpreter.define_native("starts_with", [](const std::string &str, const std::string &prefix) -> bool {
                return str.starts_with(prefix);
            });

            // check if a string contains a substring
            interpreter.define_native("contains", [](const std::string &str, const std::string &substr) -> bool {
                return str.find(substr) != std::string::npos;
            });

            // pulls a substring out using native double types
            interpreter.define_native("substring",
                                      [](const std::string &str, const double start,
                                         const double length) -> std::string {
                                          const size_t s = static_cast<size_t>(std::max(0.0, start));
                                          const size_t len = static_cast<size_t>(std::max(0.0, length));

                                          if (s >= str.length()) return "";
                                          return str.substr(s, len);
                                      });

            // trims whitespace from both ends
            interpreter.define_native("trim", [](std::string str) -> std::string {
                const auto start = std::ranges::find_if_not(str, [](const unsigned char ch) {
                    return std::isspace(ch);
                });
                const auto end = std::find_if_not(str.rbegin(), str.rend(), [](const unsigned char ch) {
                    return std::isspace(ch);
                }).base();
                return (start < end) ? std::string(start, end) : "";
            });

            // string replacement
            interpreter.define_native("replace",
                                      [](std::string str, const std::string &search_for,
                                         const std::string &replace_with) -> std::string {
                                          if (search_for.empty()) return str;
                                          size_t pos = 0;
                                          while ((pos = str.find(search_for, pos)) != std::string::npos) {
                                              str.replace(pos, search_for.length(), replace_with);
                                              pos += replace_with.length();
                                          }
                                          return str;
                                      });
        }
    };

    // SYSTEM LIBRARY
    class SystemLib : public Lib {
    public:
        void register_modules(Interpreter &interpreter) override {
            // immediately terminates the host process with a status code
            interpreter.define_native("exit", [](const double status_code) -> double {
                std::exit(static_cast<int>(status_code));
                return 0.0; // never reached but  for signature matching
            });

            std::istream *in = &interpreter.Get_Stdin();
            // whitespace delimited word
            interpreter.define_native("read", [in]() -> std::string {
                if (std::string word; *in >> word) {
                    return word;
                }
                return "";
            });

            // read a line (obviously)
            interpreter.define_native("readln", [in]() -> std::string {
                if (std::string line; std::getline(*in, line)) {
                    return line;
                }
                return "";
            });

            interpreter.define_native("get_env", [](const std::string &var_name) -> std::string {
                const char *env_p = std::getenv(var_name.c_str());
                return env_p ? std::string(env_p) : "";
            });

            // clock tracking precision
            interpreter.define_native("clock", [] {
                const auto now = std::chrono::system_clock::now().time_since_epoch();
                return static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(now).count()) / 1000.0;
            });

            // check before running read_file
            interpreter.define_native("file_exists", [](const std::string &path) -> bool {
                return std::filesystem::exists(path);
            });

            // returns the file extension of given path
            interpreter.define_native("get_file_ext", [](const std::string &path) -> std::string {
                return std::filesystem::path(path).extension().string();
            });

            // reads (text) file into a string
            interpreter.define_native("read_file", [](const std::string &path) -> std::string {
                std::ifstream file(path);
                if (!file.is_open()) {
                    throw std::runtime_error("File Error: Could not open file for reading: " + path);
                }
                std::stringstream buffer;
                buffer << file.rdbuf();
                return buffer.str();
            });

            // overwrites or creates a (text) file with string contents
            interpreter.define_native("write_file", [](const std::string &path, const std::string &content) -> bool {
                std::ofstream file(path);
                if (!file.is_open()) {
                    throw std::runtime_error("File Error: Could not open file for writing: " + path);
                }
                file << content;
                return true;
            });

            // wait function
            interpreter.define_native("wait", [](const double seconds) -> double {
                if (seconds > 0.0) {
                    std::this_thread::sleep_for(std::chrono::duration<double>(seconds));
                }
                return seconds;
            });
        }
    };

    // REFLECTION LIBRARY
    // todo : implement...
    class Reflection : public Lib {
    public:
        void register_modules(Interpreter &) override {
        }
    };
} // namespace ObSL

#endif // OBLIBERRY_STDMODULES_H
