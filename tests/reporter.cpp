#include <catch2/reporters/catch_reporter_streaming_base.hpp>
#include <catch2/catch_test_case_info.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

#include "globals.h"

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

    // void benchmarkStarting(Catch::BenchmarkInfo const& info) override {
    //     Catch::StreamingReporterBase::benchmarkStarting(info);
    // }

    // Appelé à chaque fois qu'un bloc BENCHMARK se termine
    void benchmarkEnded(Catch::BenchmarkStats<> const& benchmarkStats) override {
        Catch::StreamingReporterBase::benchmarkEnded(benchmarkStats);

        // .mean.point.count() donne la moyenne statistique calculée par Catch2 en nanosecondes
        double mean_ns = benchmarkStats.mean.point.count();
        std::string name = benchmarkStats.info.name;
        m_benchmark_results.emplace_back(name, mean_ns);

    }

    void testCaseStarting(Catch::TestCaseInfo const& testInfo) override {
        Catch::StreamingReporterBase::testCaseStarting(testInfo);
        std::cout << "==================================================\n";
        std::cout << testInfo.name << "\n";
        std::cout << "==================================================\n";
    }

    // void testCaseEnded(Catch::TestCaseStats const& testCaseStats) override {
    //     Catch::StreamingReporterBase::testCaseEnded(testCaseStats);
    // }

    void sectionStarting(Catch::SectionInfo const& sectionInfo) override {
        Catch::StreamingReporterBase::sectionStarting(sectionInfo);
        current_size = 0;
    }
    // Appelé quand la section se termine
    void sectionEnded(Catch::SectionStats const& sectionStats) override {
        Catch::StreamingReporterBase::sectionEnded(sectionStats);
        std::cout << "INPUT SIZE = " << std::to_string(current_size) << " Bytes\n";

        std::string tested_name = m_benchmark_results[0].name;

        // Si au moins deux benchmarks cibles ont été mesurés, comparer les temps
        if (m_benchmark_results.size() >= 2) {
            // trier du plus lent au plus rapide
            std::sort(m_benchmark_results.begin(), m_benchmark_results.end(), [](const BenchmarkResult& a, const BenchmarkResult& b) {
                return a.mean_ns > b.mean_ns;
            });

            double temps_ref = m_benchmark_results[0].mean_ns;
            std::string name_ref = m_benchmark_results[0].name;
            size_t count = m_benchmark_results.size();
            
            for(size_t i = 0; i < count; ++i) {
                double duration = m_benchmark_results[i].mean_ns;
                if (duration == 0) continue;
                
                std::string library_name = m_benchmark_results[i].name;
                double speed = (current_size / (duration / 1e9)) / (1024 * 1024);
                bool is_tested_lib = (library_name == tested_name);
                if (is_tested_lib) std::printf("\x1b[32m");
                if (i == 0) {
                    std::printf( "%zu: %14s :%.0f ns speed=%.3f MB/s\n", count, library_name.c_str(), duration , speed );
                } else {
                    double diff_pourcent = ((temps_ref - duration) / temps_ref) * 100.0;
                    std::printf( "%zu: %14s :%.0f ns speed=%.3f MB/s, %.2f%% faster than %s.\n", count - i, library_name.c_str(), duration, speed, diff_pourcent, name_ref.c_str()  );
                }
                if (is_tested_lib) std::printf("\x1b[0m");
            }
        }
        
        std::cout << "--------------------------------------------------\n";

        // Vider pour le prochain cas de test
        m_benchmark_results.clear();
    }
};

// Enregistrement du reporter auprès de Catch2
CATCH_REGISTER_REPORTER("perf-compare", PerformanceReporter)
