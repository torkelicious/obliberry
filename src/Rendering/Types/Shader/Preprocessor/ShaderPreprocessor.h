#pragma once
#include "Scripting/SmallFunction.h"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Rendering {

    struct PPState {
        std::vector<std::filesystem::path> dependencies;
        std::unordered_map<std::filesystem::path, int> fileToId;
        std::vector<std::filesystem::path> idToFile;

        // tracks circular includes
        std::unordered_set<std::filesystem::path> activeStack;

        // #version directive done yet
        bool versionHandled = false;

        virtual ~PPState() = default;

        int getFileId(const std::filesystem::path &path) {
            const std::filesystem::path clean = path.lexically_normal();
            if (const auto it = fileToId.find(clean); it != fileToId.end()) {
                return it->second;
            }
            const int id = static_cast<int>(idToFile.size());
            fileToId.emplace(clean, id);
            idToFile.push_back(clean);
            return id;
        }
    };

    struct BuiltinPPState : public PPState {
        std::unordered_set<std::filesystem::path> pragmaOnce;
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

        void setVirtualPathMode(bool enabled) { m_VirtualPaths = enabled; }

        std::string loadFile(const std::filesystem::path &path) const;

        const std::string &loadFileCached(const std::filesystem::path &path) const;

        std::string processSource(const std::string &source, const std::filesystem::path &path, PPState &state);
        void processSourceInto(const std::string &source, const std::filesystem::path &path, PPState &state, std::string &output);

        std::filesystem::path resolvePath(const std::filesystem::path &includePath, const std::filesystem::path &currPath) const;

    private:
        std::vector<std::filesystem::path> m_IncludeDirs;
        std::unordered_map<std::string, DirectiveFunc> m_Directives;
        FileLoaderFunc m_FileLoader;
        bool m_VirtualPaths = false;

        mutable std::unordered_map<std::string, std::string> m_SourceCache;

        static std::string_view stripWhitespace(std::string_view src);
        static void extractDirectiveAndArgs(std::string_view ln, std::string &outDir, std::string &outArgs);

        static std::string_view codePortion(std::string_view line, bool &inBlockComment, std::string &scratch);
    };

} // namespace Rendering
