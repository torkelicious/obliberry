#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace IO::Package::Tools {
    class DependencyGraph {
    public:
        void add_script(const std::string &canonical_path, std::vector<std::string> deps);

        [[nodiscard]] bool validate(const std::string &binary_name) const;

    private:
        std::unordered_map<std::string, std::vector<std::string> > m_edges;
        std::unordered_set<std::string> m_known_scripts;
    };
}
