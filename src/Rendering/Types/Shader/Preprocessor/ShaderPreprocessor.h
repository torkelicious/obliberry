#pragma once
#include "Scripting/SmallFunction.h"


#include <cstdint>
#include <filesystem>
#include <unordered_map>
#include <vector>

// basic preprocessor thing for #include and such
namespace Rendering::Shader {

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

    class Preprocessor;

    using DirectiveFunc = Scripting::SmallFunction<void(const std::string &args, const std::filesystem::path &path, uint32_t lineNumber, PPState &state, Preprocessor &processor, std::string &output)>;


    class Preprocessor {
    public:
        Preprocessor();
        void addIncludeDirectory(const std::filesystem::path &dir);
        void registerDirective(const std::string &directive, DirectiveFunc func);
        std::string processFile(const std::filesystem::path &path, PPState &state);
        std::filesystem::path resolvePath(const std::filesystem::path &includePath, const std::filesystem::path &currPath) const;

    private:
        std::vector<std::filesystem::path> m_IncludeDirs;
        std::unordered_map<std::string, DirectiveFunc> m_Directives;

        static std::string stripWhitespace(const std::string &src);
        static void extractDirectiveAndArgs(const std::string &ln, std::string &outDir, std::string &outArgs);
    };


} // namespace Rendering::Shader
