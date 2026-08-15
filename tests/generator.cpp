
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <fstream>
#include <string>
#include <vector>
#include <memory>  // Pour std::unique_ptr

class FileContentGenerator : public Catch::Generators::IGenerator<std::string> {
    std::vector<std::string> fileContents;
    size_t currentIndex = 0;

public:
    explicit FileContentGenerator(const std::vector<std::string>& filePaths) {
        for (const auto& filePath : filePaths) {
            std::ifstream file(filePath);
            if (!file.is_open()) {
                throw std::runtime_error("Impossible d'ouvrir le fichier : " + filePath);
            }
            fileContents.push_back(
                std::string(
                    (std::istreambuf_iterator<char>(file)),
                    std::istreambuf_iterator<char>()
                )
            );
        }
    }

    std::string const& get() const override {
        return fileContents[currentIndex];
    }

    bool next() override {
        if (currentIndex + 1 >= fileContents.size()) {
            return false;
        }
        ++currentIndex;
        return true;
    }
};
// Solution correcte pour Catch2 v3 : utiliser le constructeur adapté
Catch::Generators::GeneratorWrapper<std::string> fileContents(
    const std::vector<std::string>& filePaths
) {
    return Catch::Generators::GeneratorWrapper<std::string>(
        std::make_unique<FileContentGenerator>(filePaths).get()
    );
}