#include "DependencyGraph.h"
#include "CliCommon.h"
#include <ObSL/ModulePath.h>
#include <functional>

namespace IO::Package::Tools {
    static void collect_using_paths(const ObSL::Stmt *stmt, std::vector<std::string> &out) {
        if (!stmt) return;
        using ObSL::StmtType;
        switch (stmt->type()) {
            case StmtType::Using:
                out.push_back(static_cast<const ObSL::UsingStmt *>(stmt)->path);
                break;
            case StmtType::Block:
                for (auto &s: static_cast<const ObSL::BlockStmt *>(stmt)->statements)
                    collect_using_paths(s.get(), out);
                break;
            case StmtType::If: {
                auto *n = static_cast<const ObSL::IfStmt *>(stmt);
                collect_using_paths(n->then_branch.get(), out);
                collect_using_paths(n->else_branch.get(), out);
                break;
            }
            case StmtType::While:
                collect_using_paths(static_cast<const ObSL::WhileStmt *>(stmt)->body.get(), out);
                break;
            case StmtType::Foreach:
                collect_using_paths(static_cast<const ObSL::ForeachStmt *>(stmt)->body.get(), out);
                break;
            case StmtType::Function:
                collect_using_paths(static_cast<const ObSL::FunctionStmt *>(stmt)->body.get(), out);
                break;
            case StmtType::Switch:
                for (auto &c: static_cast<const ObSL::SwitchStmt *>(stmt)->cases)
                    for (auto &s: c.statements) collect_using_paths(s.get(), out);
                break;
            case StmtType::TryCatch: {
                auto *n = static_cast<const ObSL::TryCatchStmt *>(stmt);
                collect_using_paths(n->try_body.get(), out);
                collect_using_paths(n->catch_body.get(), out);
                break;
            }
            default:
                break;
        }
    }

    void DependencyGraph::add_script(const std::string &canonical_path,
                                     const std::vector<std::unique_ptr<ObSL::Stmt> > &ast,
                                     const std::filesystem::path &project_root) {
        m_known_scripts.insert(canonical_path);
        std::vector<std::string> raw_uses;
        for (auto &stmt: ast) collect_using_paths(stmt.get(), raw_uses);

        auto &deps = m_edges[canonical_path];
        for (auto &raw: raw_uses) {
            auto dep = ObSL::canonicalize_module_path(project_root, raw);
            deps.push_back(std::filesystem::relative(std::filesystem::path(dep), project_root).generic_string());
        }
    }

    bool DependencyGraph::validate(const std::string &binary_name) const {
        bool ok = true;

        // missingmodule check
        for (const auto &[script, deps]: m_edges) {
            for (const auto &dep: deps) {
                if (!m_known_scripts.contains(dep)) {
                    log_error(binary_name, script + " -> using target not found: " + dep);
                    ok = false;
                }
            }
        }

        // cycle detection (DFS 3-color)
        std::unordered_map<std::string, int> color; // 0=white, 1=gray, 2=black
        std::function < bool(const std::string &, std::vector<std::string> &) > has_cycle =
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
