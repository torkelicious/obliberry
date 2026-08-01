#include "IgnoreRules.h"
#include <fstream>
#include <iterator>
#include <utility>
#include <string>

namespace IO::Package::Tools {
    namespace {
        // escapes one character
        std::string escape_regex_char(const char c) {
            static constexpr std::string_view kRegexSpecial = R"(\.^$|?*+()[]{}-)";
            if (kRegexSpecial.find(c) != std::string_view::npos)
                return std::string("\\") + c;
            return std::string(1, c);
        }

        // translate to regex
        std::string segment_to_regex(const std::string_view seg) {
            std::string rx;
            rx.reserve(seg.size() * 2);
            for (size_t i = 0; i < seg.size(); ++i) {
                const char c = seg[i];
                if (c == '*') {
                    rx += "[^/]*";
                } else if (c == '?') {
                    rx += "[^/]";
                } else if (c == '\\' && i + 1 < seg.size()) {
                    rx += escape_regex_char(seg[++i]);
                } else {
                    rx += escape_regex_char(c);
                }
            }
            return rx;
        }

        std::string pattern_to_regex(std::string_view pattern, const bool anchored) {
            // split the pattern into '/' separated segments.
            std::vector<std::string_view> segs;
            for (size_t start = 0;;) {
                const size_t slash = pattern.find('/', start);
                if (slash == std::string_view::npos) {
                    segs.push_back(pattern.substr(start));
                    break;
                }
                segs.push_back(pattern.substr(start, slash - start));
                start = slash + 1;
            }

            if (!anchored) {
                return "^(?:.*/)?" + segment_to_regex(segs.front()) + "$";
            }

            std::string rx = "^";
            for (size_t i = 0; i < segs.size(); ++i) {
                if (segs[i] == "**") {
                    // '**' matches zero or more directory levels
                    if (i + 1 < segs.size())
                        rx += "(?:[^/]+/)*";
                    else
                        rx += "(?:[^/]+/)*[^/]*";
                } else {
                    rx += segment_to_regex(segs[i]);
                    if (i + 1 < segs.size())
                        rx += '/';
                }
            }
            rx += "$";
            return rx;
        }
    } // namespace

    IgnoreRules IgnoreRules::ForProject(const std::filesystem::path &project_dir) {
        IgnoreRules rules;
        rules.base_dir_ = project_dir;
        // built in defaults are kept first so a .pakignore entry can re-include, maybe just make a default file later though idk
        rules.AddPattern("imgui.ini");
        rules.AddPattern(".DS_Store");
        rules.AddPattern("graphics.json");
        rules.AddPattern(".pakignore");
        IgnoreRules file_rules = Load(project_dir / ".pakignore");
        rules.rules_.insert(rules.rules_.end(), std::make_move_iterator(file_rules.rules_.begin()), std::make_move_iterator(file_rules.rules_.end()));
        return rules;
    }

    IgnoreRules IgnoreRules::Load(const std::filesystem::path &ignore_file) {
        IgnoreRules rules;
        rules.base_dir_ = ignore_file.parent_path();
        std::ifstream in(ignore_file);
        if (!in)
            return rules;
        std::string line;
        while (std::getline(in, line)) {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            rules.AddPattern(line);
        }
        return rules;
    }

    void IgnoreRules::AddPattern(std::string_view line) {
        if (line.empty())
            return;

        bool negate = false;
        if (line.front() == '!') {
            negate = true;
            line.remove_prefix(1);
            if (line.empty())
                return;
        }

        // '#' or '!' escaped with a leading backslash are literals
        if (line.size() >= 2 && line.front() == '\\' && (line[1] == '#' || line[1] == '!'))
            line.remove_prefix(1);

        bool dir_only = false;
        if (line.back() == '/') {
            dir_only = true;
            line.remove_suffix(1);
            if (line.empty())
                return;
        }

        const bool anchored = line.front() == '/';
        if (anchored)
            line.remove_prefix(1);
        if (line.empty())
            return;

        // matched against the full relative path
        const bool has_slash = line.find('/') != std::string_view::npos;

        Rule rule;
        rule.negate = negate;
        rule.dir_only = dir_only;
        rule.regex = std::regex(pattern_to_regex(line, anchored || has_slash));
        rules_.push_back(std::move(rule));
    }

    bool IgnoreRules::IsIgnored(const std::filesystem::path &path, const bool is_dir) const {
        if (rules_.empty() || base_dir_.empty())
            return false;

        std::error_code ec;
        const std::filesystem::path rel = std::filesystem::relative(path, base_dir_, ec);
        if (ec)
            return false;
        std::string rel_str = rel.generic_string();
        if (rel_str == ".")
            rel_str.clear();
        if (rel_str == ".." || rel_str.starts_with("../"))
            return false; // outside the ignore file directory

        bool ignored = false;
        for (const auto &rule : rules_) {
            if (rule.dir_only && !is_dir)
                continue;
            if (std::regex_match(rel_str, rule.regex))
                ignored = !rule.negate;
        }
        return ignored;
    }
} // namespace IO::Package::Tools
