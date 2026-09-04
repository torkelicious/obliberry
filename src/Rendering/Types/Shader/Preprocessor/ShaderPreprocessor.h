#pragma once
#include "Scripting/SmallFunction.h"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Rendering {

    struct PPState {
        std::vector<std::filesystem::path> dependencies;
        std::unordered_map<std::filesystem::path, int> fileToId;
        std::vector<std::filesystem::path> idToFile;

        virtual ~PPState() = default;

        int getFileId(const std::filesystem::path &path) {
            const std::filesystem::path clean = std::filesystem::weakly_canonical(path);
            if (!fileToId.contains(clean)) {
                fileToId[clean] = static_cast<int>(idToFile.size());
                idToFile.push_back(clean);
            }
            return fileToId[clean];
        }
    };

    struct BuiltinPPState : public PPState {
        std::unordered_set<std::filesystem::path> pragmaOnce;
        std::unordered_set<std::filesystem::path> activeStack;
    };

    class ShaderPreprocessor;

    using DirectiveFunc = Scripting::SmallFunction<void(const std::string &args, const std::filesystem::path &path, uint32_t lineNumber, PPState &state, ShaderPreprocessor &processor, std::string &output)>;
    using FileLoaderFunc = std::function<std::string(const std::filesystem::path &)>;

    class ShaderPreprocessor {
    public:
        static ShaderPreprocessor &Get() {
            static ShaderPreprocessor instance;
            return instance;
        }

        ShaderPreprocessor();

        // pls only call on main thread >w<
        void addIncludeDirectory(const std::filesystem::path &dir);
        void registerDirective(const std::string &directive, DirectiveFunc func);

        void setFileLoader(FileLoaderFunc loader);
        std::string loadFile(const std::filesystem::path &path) const;

        std::string processSource(const std::string &source, const std::filesystem::path &path, PPState &state);
        std::filesystem::path resolvePath(const std::filesystem::path &includePath, const std::filesystem::path &currPath) const;

    private:
        std::vector<std::filesystem::path> m_IncludeDirs;
        std::unordered_map<std::string, DirectiveFunc> m_Directives;
        FileLoaderFunc m_FileLoader;

        static std::string stripWhitespace(const std::string &src);
        static void extractDirectiveAndArgs(const std::string &ln, std::string &outDir, std::string &outArgs);
    };

} // namespace Rendering
