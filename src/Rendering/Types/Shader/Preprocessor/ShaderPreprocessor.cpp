#include "ShaderPreprocessor.h"
#include <fstream>
#include <unordered_set>

namespace Rendering::Shader {

    namespace {
        struct BuiltinPPState : public PPState {
            std::unordered_set<std::filesystem::path> pragmaOnce;
            std::unordered_set<std::filesystem::path> activeStack;
        };
    } // namespace


    // register #include and #pragma once
    Preprocessor::Preprocessor() {
        registerDirective("include", [](const std::string &args, const std::filesystem::path &path, const uint32_t lineNum, PPState &baseState, Preprocessor &proc, std::string &output) {
            auto *extState = dynamic_cast<BuiltinPPState *>(&baseState);
            std::filesystem::path includePath;

            if (args.size() > 2 && ((args.front() == '"' && args.back() == '"') || (args.front() == '<' && args.back() == '>'))) {
                includePath = args.substr(1, args.length() - 2);
            } else {
                includePath = args;
            }

            const std::filesystem::path resolved = proc.resolvePath(includePath, path);

            if (extState) {
                if (extState->activeStack.contains(resolved)) {
                    throw std::runtime_error("Circular include detected: " + resolved.string());
                }
                // if in pragmaOnce do nothing
                if (!extState->pragmaOnce.contains(resolved)) {
                    extState->activeStack.insert(resolved);
                    extState->dependencies.push_back(resolved);
                    output += proc.processFile(resolved, baseState);
                    extState->activeStack.erase(resolved);
                }
            } else {
                // for minimal state
                baseState.dependencies.push_back(resolved);
                output += proc.processFile(resolved, baseState);
            }

            output += "\n#line " + std::to_string(lineNum + 1) + " " + std::to_string(baseState.getFileId(path)) + "\n";
        });

        registerDirective("pragma", [](const std::string &args, const std::filesystem::path &path, uint32_t /*lineNum*/, PPState &baseState, Preprocessor & /*proc*/, std::string &output) {
            auto *extState = dynamic_cast<BuiltinPPState *>(&baseState);

            if (extState && args == "once") {
                extState->pragmaOnce.insert(std::filesystem::weakly_canonical(path));
            } else {
                // passthrough
                output += "#pragma " + args + "\n";
            }
        });
    }


    void Preprocessor::addIncludeDirectory(const std::filesystem::path &dir) { m_IncludeDirs.push_back(dir); }

    void Preprocessor::registerDirective(const std::string &directive, DirectiveFunc func) { m_Directives[directive] = std::move(func); }

    std::filesystem::path Preprocessor::resolvePath(const std::filesystem::path &includePath, const std::filesystem::path &currPath) const {
        if (const std::filesystem::path local = currPath.parent_path() / includePath; std::filesystem::exists(local)) {
            return std::filesystem::weakly_canonical(local);
        }
        for (const auto &dir : m_IncludeDirs) {
            if (std::filesystem::path global = dir / includePath; std::filesystem::exists(global)) {
                return std::filesystem::weakly_canonical(global);
            }
        }
        throw std::runtime_error("Could not resolve include path: " + includePath.string());
    }

    std::string Preprocessor::processFile(const std::filesystem::path &path, PPState &state) {
        std::ifstream file(path);
        if (!file.is_open()) {
            throw std::runtime_error("failed to open file: " + path.string());
        }

        int fileId = state.getFileId(path);
        std::string output = "#line 1 " + std::to_string(fileId) + "\n";
        std::string line;
        uint32_t lineNum = 1;
        while (std::getline(file, line)) {
            std::string trimmed = stripWhitespace(line);
            if (!trimmed.empty() && trimmed[0] == '#') {
                std::string directive;
                std::string args;
                extractDirectiveAndArgs(trimmed, directive, args);

                if (auto it = m_Directives.find(directive); it != m_Directives.end()) {
                    it->second(args, path, lineNum, state, *this, output);
                } else {
                    output += line + "\n"; // passtrough
                }
            } else {
                output += line + "\n"; // passthrough
            }
            lineNum++;
        }
        return output;
    }

    std::string Preprocessor::stripWhitespace(const std::string &src) {
        const size_t start = src.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) {
            return "";
        }
        const size_t end = src.find_last_not_of(" \t\r\n");
        return src.substr(start, end - start + 1);
    }

    void Preprocessor::extractDirectiveAndArgs(const std::string &ln, std::string &outDir, std::string &outArgs) {
        if (const size_t spacePos = ln.find_first_of(" \t"); spacePos == std::string::npos) {
            outDir = ln.substr(1);
            outArgs = "";
        } else {
            outDir = ln.substr(1, spacePos - 1);
            outArgs = stripWhitespace(ln.substr(spacePos));
        }
    }

} // namespace Rendering::Shader
