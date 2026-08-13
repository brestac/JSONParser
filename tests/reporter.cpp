#include <catch2/reporters/catch_reporter_streaming_base.hpp>
#include <catch2/catch_test_case_info.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

#include <iostream>
#include <vector>
#include <string>

struct BenchmarkResult {
    std::string name;
    double mean_ns;

    BenchmarkResult(std::string name, double mean_ns) : name(name), mean_ns(mean_ns) {}
};

class PerformanceReporter : public Catch::StreamingReporterBase {
private:
    // Stocke les temps moyens pour le TEST_CASE en cours
    std::vector<BenchmarkResult> m_benchmark_results;

public:
    using StreamingReporterBase::StreamingReporterBase;

    // Nom unique du reporter pour la ligne de commande
    static std::string getDescription() {
        return "Reporter personnalisé pour comparer la vitesse relative des librairies";
    }

    void benchmarkStarting(Catch::BenchmarkInfo const& info) override {
        Catch::StreamingReporterBase::benchmarkStarting(info);
    }

    // Appelé à chaque fois qu'un bloc BENCHMARK se termine
    void benchmarkEnded(Catch::BenchmarkStats<> const& benchmarkStats) override {
        // .mean.point.count() donne la moyenne statistique calculée par Catch2 en nanosecondes
        double mean_ns = benchmarkStats.mean.point.count();
        std::string name = benchmarkStats.info.name;
        m_benchmark_results.emplace_back(name, mean_ns);
        //std::printf("Benchmark %14s : %.0f ns\n", name.c_str(), mean_ns);
        // On laisse le comportement par défaut afficher le joli tableau standard
        // For some reason the default table does not display in output. Why is that ?
        Catch::StreamingReporterBase::benchmarkEnded(benchmarkStats); // < this has no effect even with --success cmdl option
    }

    void testCaseStarting(Catch::TestCaseInfo const& testInfo) override {
        Catch::StreamingReporterBase::testCaseStarting(testInfo);
    }

    void testCaseEnded(Catch::TestCaseStats const& testCaseStats) override {
        Catch::StreamingReporterBase::testCaseEnded(testCaseStats);
    }

    void sectionStarting(Catch::SectionInfo const& sectionInfo) override {
        std::cout << sectionInfo.name << "\n";
        Catch::StreamingReporterBase::sectionStarting(sectionInfo);
    }
    // Appelé quand la section se termine
    void sectionEnded(Catch::SectionStats const& sectionStats) override {
        // Si nos deux benchmarks cibles ont été mesurés
        if (m_benchmark_results.size() >= 2 && m_benchmark_results[0].name == "ce parser") {
            double temps_ref = m_benchmark_results[0].mean_ns;
            std::string librairie_ref = m_benchmark_results[0].name;
            for(size_t i = 0; i < m_benchmark_results.size(); ++i) {
                double temps_b = m_benchmark_results[i].mean_ns;
                std::string library_name = m_benchmark_results[i].name;
                if (i == 0) {
                    std::printf( "%14s :%.0f ns\n", library_name.c_str(), temps_b );
                }
                else if (temps_ref < temps_b) {
                    double diff_pourcent = ((temps_b - temps_ref) / temps_b) * 100.0;
                    std::printf( "%14s :%.0f ns. %s est %.2f%% plus rapide que %s.\n", library_name.c_str(), temps_b, librairie_ref.c_str(), diff_pourcent, library_name.c_str()  );
                } else {
                    double diff_pourcent = ((temps_ref - temps_b) / temps_ref) * 100.0;
                    std::printf( "%14s :%.0f ns est %.2f%% plus rapide que %s.\n", library_name.c_str(), temps_b, diff_pourcent, librairie_ref.c_str()  );
                }
            }
        } else {
            std::cout << "Pas assez de données pour comparer les performances.\n";
        }
        std::cout << "==================================================\n\n";

        // Vider pour le prochain cas de test
        m_benchmark_results.clear();
        Catch::StreamingReporterBase::sectionEnded(sectionStats);
    }
};

// Enregistrement du reporter auprès de Catch2
CATCH_REGISTER_REPORTER("perf-compare", PerformanceReporter)
