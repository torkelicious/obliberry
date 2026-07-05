#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <filesystem>
#include <ObSL/Parser/ast.h>

namespace IO::Package::Tools {
    class DependencyGraph {
    public:
        void add_script(const std::string &canonical_path,
                        const std::vector<std::unique_ptr<ObSL::Stmt> > &ast,
                        const std::filesystem::path &project_root);

        [[nodiscard]] bool validate(const std::string &binary_name_for_logging) const;

    private:
        std::unordered_map<std::string, std::vector<std::string> > m_edges;
        std::unordered_set<std::string> m_known_scripts;
    };
}
