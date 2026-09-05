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

            if (baseState.activeStack.contains(resolved)) {
                LOG_ERROR(LOG_WHO, "Circular include detected: " + resolved.string());
                return;
            }
            if (extState && extState->pragmaOnce.contains(resolved)) {
                return;
            }

            baseState.activeStack.insert(resolved);
            baseState.dependencies.push_back(resolved);

            const std::string &includeSource = proc.loadFileCached(resolved);
            proc.processSourceInto(includeSource, resolved, baseState, output);

            baseState.activeStack.erase(resolved);

            output += "#line " + std::to_string(lineNum + 1) + " " + std::to_string(baseState.getFileId(path)) + "\n";
        });

        registerDirective("pragma", [](const std::string &args, const std::filesystem::path &path, uint32_t /*lineNum*/, PPState &baseState, ShaderPreprocessor & /*proc*/, std::string &output) {
            auto *extState = dynamic_cast<BuiltinPPState *>(&baseState);

            if (extState && args == "once") {
                extState->pragmaOnce.insert(path.lexically_normal());
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

    const std::string &ShaderPreprocessor::loadFileCached(const std::filesystem::path &path) const {
        const std::string key = path.lexically_normal().generic_string();
        if (const auto it = m_SourceCache.find(key); it != m_SourceCache.end()) {
            return it->second;
        }
        return m_SourceCache.emplace(key, loadFile(path)).first->second;
    }

    void ShaderPreprocessor::addIncludeDirectory(const std::filesystem::path &dir) { m_IncludeDirs.push_back(dir); }

    void ShaderPreprocessor::registerDirective(const std::string &directive, DirectiveFunc func) { m_Directives[directive] = std::move(func); }

    std::filesystem::path ShaderPreprocessor::resolvePath(const std::filesystem::path &includePath, const std::filesystem::path &currPath) const {
        if (m_VirtualPaths) {
            return (currPath.parent_path() / includePath).lexically_normal();
        }

        if (const std::filesystem::path local = currPath.parent_path() / includePath; std::filesystem::exists(local)) {
            return std::filesystem::weakly_canonical(local);
        }
        for (const auto &dir : m_IncludeDirs) {
            if (std::filesystem::path global = dir / includePath; std::filesystem::exists(global)) {
                return std::filesystem::weakly_canonical(global);
            }
        }

        // fallback
        return (currPath.parent_path() / includePath).lexically_normal();
    }

    std::string ShaderPreprocessor::processSource(const std::string &source, const std::filesystem::path &path, PPState &state) {
        m_SourceCache.clear();

        std::string output;
        output.reserve(source.size() + 256);
        processSourceInto(source, path, state, output);
        return output;
    }

    void ShaderPreprocessor::processSourceInto(const std::string &source, const std::filesystem::path &path, PPState &state, std::string &output) {
        if (source.empty()) {
            return;
        }

        const int fileId = state.getFileId(path);
        const size_t startOffset = output.size();
        output.reserve(output.size() + source.size() + 64);

        bool foundFirstToken = false;
        bool inBlockComment = false;
        uint32_t lineNum = 1;

        std::string scratch; // reused buffer for comment stripping

        size_t pos = 0;
        const size_t len = source.size();
        while (pos <= len) {
            size_t nl = source.find('\n', pos);
            if (nl == std::string::npos) {
                nl = len;
            }
            std::string_view line(source.data() + pos, nl - pos);
            pos = nl + 1;

            // strip UTF-8 BOM
            if (lineNum == 1 && line.size() >= 3 && static_cast<unsigned char>(line[0]) == 0xEF && static_cast<unsigned char>(line[1]) == 0xBB && static_cast<unsigned char>(line[2]) == 0xBF) {
                line.remove_prefix(3);
            }

            const std::string_view code = codePortion(line, inBlockComment, scratch);
            const std::string_view trimmed = stripWhitespace(code);

            const bool isDirective = !trimmed.empty() && trimmed.front() == '#';

            if (!foundFirstToken && !trimmed.empty()) {
                foundFirstToken = true;
                if (isDirective) {
                    std::string dir;
                    std::string args;
                    extractDirectiveAndArgs(trimmed, dir, args);
                    if (dir != "version") {
                        output += "#line " + std::to_string(lineNum) + " " + std::to_string(fileId) + "\n";
                    }
                } else {
                    output += "#line " + std::to_string(lineNum) + " " + std::to_string(fileId) + "\n";
                }
            }

            // processing
            if (isDirective) {
                std::string directive;
                std::string args;
                extractDirectiveAndArgs(trimmed, directive, args);

                if (directive == "version") {
                    if (!state.versionHandled) {
                        state.versionHandled = true;
                        output.append(line);
                        output.push_back('\n');
                    }
                    output += "#line " + std::to_string(lineNum + 1) + " " + std::to_string(fileId) + "\n";
                } else if (auto it = m_Directives.find(directive); it != m_Directives.end()) {
                    it->second(args, path, lineNum, state, *this, output);
                } else {
                    output.append(line);
                    output.push_back('\n');
                }
            } else {
                output.append(line);
                output.push_back('\n');
            }
            lineNum++;
        }

        if (!foundFirstToken) {
            output.insert(startOffset, "#line 1 " + std::to_string(fileId) + "\n");
        }
    }


    std::string_view ShaderPreprocessor::stripWhitespace(const std::string_view src) {
        const size_t start = src.find_first_not_of(" \t\r\n");
        if (start == std::string_view::npos) {
            return {};
        }
        const size_t end = src.find_last_not_of(" \t\r\n");
        return src.substr(start, end - start + 1);
    }

    void ShaderPreprocessor::extractDirectiveAndArgs(const std::string_view ln, std::string &outDir, std::string &outArgs) {
        if (const size_t spacePos = ln.find_first_of(" \t"); spacePos == std::string_view::npos) {
            outDir.assign(ln.substr(1));
            outArgs.clear();
        } else {
            outDir.assign(ln.substr(1, spacePos - 1));
            outArgs.assign(stripWhitespace(ln.substr(spacePos)));
        }
    }

    std::string_view ShaderPreprocessor::codePortion(const std::string_view line, bool &inBlockComment, std::string &scratch) {
        if (!inBlockComment && line.find('/') == std::string_view::npos) {
            return line;
        }

        scratch.clear();
        size_t i = 0;
        while (i < line.size()) {
            if (inBlockComment) {
                const size_t end = line.find("*/", i);
                if (end == std::string_view::npos) {
                    return scratch;
                }
                inBlockComment = false;
                i = end + 2;
                continue;
            }

            const size_t slash = line.find("//", i);
            const size_t block = line.find("/*", i);
            if (slash == std::string_view::npos && block == std::string_view::npos) {
                scratch.append(line.substr(i));
                break;
            }
            if (slash != std::string_view::npos && (block == std::string_view::npos || slash < block)) {
                scratch.append(line.substr(i, slash - i));
                break;
            }
            scratch.append(line.substr(i, block - i));
            inBlockComment = true;
            i = block + 2;
        }
        return scratch;
    }

} // namespace Rendering
#pragma pop_macro("LOG_WHO")
