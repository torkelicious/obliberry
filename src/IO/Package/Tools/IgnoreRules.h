#pragma once
#include <filesystem>
#include <regex>
#include <string_view>
#include <vector>

namespace IO::Package::Tools {
    //   # comment
    //   !pattern             negates (re-includes) a previously ignored path
    //   pattern/             matches directories only
    //   /pattern             anchored to the ignore file's directory
    //   pattern with '/'     matched against the full path relative to the ignore file
    //   pattern without '/'  matched against the basename at any depth
    //   * ? **               glob wildcards ('**' crosses directory boundaries)
    //   \x                   escapes a literal 'x'
    //
    // evaluated in order and the last matching rule wins
    class IgnoreRules {
    public:
        // paths are interpreted relative to project_dir
        static IgnoreRules ForProject(const std::filesystem::path &project_dir);

        static IgnoreRules Load(const std::filesystem::path &ignore_file);

        void AddPattern(std::string_view line);

        [[nodiscard]] bool IsIgnored(const std::filesystem::path &path, bool is_dir) const;

    private:
        struct Rule {
            std::regex regex;
            bool negate = false;
            bool dir_only = false;
        };

        std::filesystem::path base_dir_;
        std::vector<Rule> rules_;
    };
} // namespace IO::Package::Tools
