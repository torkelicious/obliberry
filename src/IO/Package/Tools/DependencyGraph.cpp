#include "DependencyGraph.h"
#include "CliCommon.h"
#include <functional>

namespace IO::Package::Tools {
    void DependencyGraph::add_script(const std::string &canonical_path, std::vector<std::string> deps) {
        m_known_scripts.insert(canonical_path);
        m_edges[canonical_path] = std::move(deps);
    }

    bool DependencyGraph::validate(const std::string &binary_name) const {
        bool ok = true;

        // missing-module check
        for (const auto &[script, deps]: m_edges) {
            for (const auto &dep: deps) {
                if (!m_known_scripts.contains(dep)) {
                    log_error(binary_name, script + " -> using target not found: " + dep);
                    ok = false;
                }
            }
        }

        // cycle detection (DFS, 3-color)
        std::unordered_map<std::string, int> color; // 0=white, 1=gray, 2=black
        std::function<bool(const std::string &, std::vector<std::string> &)> has_cycle =
                [&](const std::string &node, std::vector<std::string> &path) -> bool {
            color[node] = 1;
            path.push_back(node);
            if (const auto it = m_edges.find(node); it != m_edges.end()) {
                for (const auto &dep: it->second) {
                    if (!m_known_scripts.contains(dep)) continue; // already reported above
                    if (color[dep] == 1) {
                        path.push_back(dep);
                        return true;
                    }
                    if (color[dep] == 0 && has_cycle(dep, path)) return true;
                }
            }
            path.pop_back();
            color[node] = 2;
            return false;
        };

        for (const auto &script: m_known_scripts) {
            if (color[script] == 0) {
                std::vector<std::string> path;
                if (has_cycle(script, path)) {
                    std::string chain;
                    for (auto &p: path) chain += p + " -> ";
                    chain += path.front();
                    log_error(binary_name, "Circular using dependency: " + chain);
                    ok = false;
                }
            }
        }
        return ok;
    }
} // namespace IO::Package::Tools
