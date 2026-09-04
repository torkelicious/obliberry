#include "ShaderPreprocessor.h"
#include "Logger/LoggerService.h"
#include <fstream>
#include <sstream>

#pragma push_macro("LOG_WHO")
#define LOG_WHO "ShaderPreprocessor"

namespace Rendering {

    ShaderPreprocessor::ShaderPreprocessor() {
        // default standalone thingamabob since watever i may wanna use ts decoupled type shi later
        m_FileLoader = [](const std::filesystem::path &p) {
            std::ifstream file(p);
            if (!file.is_open()) {
                LOG_ERROR(LOG_WHO, "Failed to open file: " + p.string());
                return std::string("");
            }
            std::stringstream buffer;
            buffer << file.rdbuf();
            return buffer.str();
        };

        registerDirective("include", [](const std::string &args, const std::filesystem::path &path, const uint32_t lineNum, PPState &baseState, ShaderPreprocessor &proc, std::string &output) {
            auto *extState = dynamic_cast<BuiltinPPState *>(&baseState);
            std::filesystem::path includePath;

            if (args.size() > 2 && ((args.front() == '"' && args.back() == '"') || (args.front() == '<' && args.back() == '>'))) {
                includePath = args.substr(1, args.length() - 2);
            } else {
                includePath = args;
            }

            const std::filesystem::path resolved = proc.resolvePath(includePath, path);
            if (resolved.empty()) {
                return; // abort mission !!!
            }

            if (extState) {
                if (extState->activeStack.contains(resolved)) {
                    LOG_ERROR(LOG_WHO, "Circular include detected: " + resolved.string());
                    return;
                }
                if (!extState->pragmaOnce.contains(resolved)) {
                    extState->activeStack.insert(resolved);
                    extState->dependencies.push_back(resolved);

                    std::string includeSource = proc.loadFile(resolved);
                    output += proc.processSource(includeSource, resolved, baseState);

                    extState->activeStack.erase(resolved);
                }
            } else {
                baseState.dependencies.push_back(resolved);

                std::string includeSource = proc.loadFile(resolved);
                output += proc.processSource(includeSource, resolved, baseState);
            }

            output += "\n#line " + std::to_string(lineNum + 1) + " " + std::to_string(baseState.getFileId(path)) + "\n";
        });

        registerDirective("pragma", [](const std::string &args, const std::filesystem::path &path, uint32_t /*lineNum*/, PPState &baseState, ShaderPreprocessor & /*proc*/, std::string &output) {
            auto *extState = dynamic_cast<BuiltinPPState *>(&baseState);

            if (extState && args == "once") {
                extState->pragmaOnce.insert(std::filesystem::weakly_canonical(path));
            } else {
                output += "#pragma " + args + "\n";
            }
        });
    }

    void ShaderPreprocessor::setFileLoader(FileLoaderFunc loader) { m_FileLoader = std::move(loader); }

    std::string ShaderPreprocessor::loadFile(const std::filesystem::path &path) const {
        if (m_FileLoader) {
            return m_FileLoader(path);
        }
        LOG_ERROR(LOG_WHO, "No file loader configured for ShaderPreprocessor.");
        return "";
    }

    void ShaderPreprocessor::addIncludeDirectory(const std::filesystem::path &dir) { m_IncludeDirs.push_back(dir); }

    void ShaderPreprocessor::registerDirective(const std::string &directive, DirectiveFunc func) { m_Directives[directive] = std::move(func); }

    std::filesystem::path ShaderPreprocessor::resolvePath(const std::filesystem::path &includePath, const std::filesystem::path &currPath) const {
        if (const std::filesystem::path local = currPath.parent_path() / includePath; std::filesystem::exists(local)) {
            return std::filesystem::weakly_canonical(local);
        }
        for (const auto &dir : m_IncludeDirs) {
            if (std::filesystem::path global = dir / includePath; std::filesystem::exists(global)) {
                return std::filesystem::weakly_canonical(global);
            }
        }

        // fallback
        return currPath.parent_path() / includePath;
    }

    std::string ShaderPreprocessor::processSource(const std::string &source, const std::filesystem::path &path, PPState &state) {
        if (source.empty()) {
            return "";
        }

        int fileId = state.getFileId(path);
        std::string output;

        std::istringstream stream(source);
        std::string line;
        uint32_t lineNum = 1;

        bool foundFirstToken = false;
        bool inBlockComment = false;

        while (std::getline(stream, line)) {
            // strip UTF-8 BOM
            if (lineNum == 1 && line.size() >= 3 && static_cast<unsigned char>(line[0]) == 0xEF && static_cast<unsigned char>(line[1]) == 0xBB && static_cast<unsigned char>(line[2]) == 0xBF) {
                line = line.substr(3);
            }

            std::string trimmed = stripWhitespace(line);

            // skip comments and blank lines to actaul start of the file
            if (inBlockComment) {
                if (trimmed.find("*/") != std::string::npos) {
                    inBlockComment = false;
                }
            } else {
                if (!foundFirstToken && !trimmed.empty()) {
                    if (trimmed.find("//") == 0) {
                    } else if (trimmed.find("/*") == 0) {
                        if (trimmed.find("*/") == std::string::npos) {
                            inBlockComment = true;
                        }
                    } else {
                        // first real line
                        foundFirstToken = true;
                        if (trimmed[0] == '#') {
                            std::string dir;
                            std::string args;
                            extractDirectiveAndArgs(trimmed, dir, args);

                            if (dir == "version") {
                                output += line + "\n";
                                // start tracking
                                output += "#line " + std::to_string(lineNum + 1) + " " + std::to_string(fileId) + "\n";
                                lineNum++;
                                continue;
                            }
                        }
                        output += "#line " + std::to_string(lineNum) + " " + std::to_string(fileId) + "\n";
                    }
                }
            }

            // processing
            if (!trimmed.empty() && trimmed[0] == '#') {
                std::string directive;
                std::string args;
                extractDirectiveAndArgs(trimmed, directive, args);

                if (auto it = m_Directives.find(directive); it != m_Directives.end()) {
                    it->second(args, path, lineNum, state, *this, output);
                } else {
                    output += line + "\n";
                }
            } else {
                output += line + "\n";
            }
            lineNum++;
        }

        if (!foundFirstToken) {
            output = "#line 1 " + std::to_string(fileId) + "\n" + output;
        }

        return output;
    }


    std::string ShaderPreprocessor::stripWhitespace(const std::string &src) {
        const size_t start = src.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) {
            return "";
        }
        const size_t end = src.find_last_not_of(" \t\r\n");
        return src.substr(start, end - start + 1);
    }

    void ShaderPreprocessor::extractDirectiveAndArgs(const std::string &ln, std::string &outDir, std::string &outArgs) {
        if (const size_t spacePos = ln.find_first_of(" \t"); spacePos == std::string::npos) {
            outDir = ln.substr(1);
            outArgs = "";
        } else {
            outDir = ln.substr(1, spacePos - 1);
            outArgs = stripWhitespace(ln.substr(spacePos));
        }
    }

} // namespace Rendering
#pragma pop_macro("LOG_WHO")
