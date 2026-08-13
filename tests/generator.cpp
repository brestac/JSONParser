#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// Générateur pour lire le contenu d'un fichier complet
class WholeFileGenerator : public Catch::Generators::IGenerator<std::string> {
    std::string m_fileContent;
    bool m_hasMore = true;

public:
    explicit WholeFileGenerator(std::string const& filepath) {
        std::ifstream stream(filepath);
        if (stream) {
            std::stringstream buffer;
            buffer << stream.rdbuf();
            m_fileContent = buffer.str();
        } else {
            m_fileContent = "ERROR: Fichier introuvable (" + filepath + ")";
        }
    }

    std::string const& get() const override { return m_fileContent; }

    bool next() override {
        if (m_hasMore) {
            m_hasMore = false;
            return true;
        }
        return false;
    }
};

// Catch::Generators::GeneratorWrapper<std::string> wholeFile(std::string const& filepath) {
//     return Catch::Generators::makeGenerators<WholeFileGenerator>(filepath);
// }

Catch::Generators::GeneratorWrapper<std::string> wholeFile(std::string const& filepath) {
    return Catch::Generators::GeneratorWrapper<std::string>(
        new WholeFileGenerator(filepath)
    );
}
// Fonction utilitaire pour récupérer tous les chemins réguliers d'un dossier
std::vector<std::string> getFilesFromDirectory(std::string const& dirPath) {
    std::vector<std::string> files;
    if (fs::exists(dirPath) && fs::is_directory(dirPath)) {
        for (auto const& entry : fs::directory_iterator(dirPath)) {
            if (entry.is_regular_file()) {
                files.push_back(entry.path().string());
            }
        }
    }
    return files;
}
