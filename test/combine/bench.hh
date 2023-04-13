/**
 * \file bench.hh
 * \brief Primitive benchmarking for Combine tests
 */

// Standard library
#include <chrono>

// NOA headers
#include <noa/utils/combine/combine.hh>

#define BMARK(what, ...) \
    {\
        auto start = std::chrono::high_resolution_clock::now();\
        what(__VA_ARGS__);\
        auto end = std::chrono::high_resolution_clock::now();\
        auto ms_int = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);\
        std::cout << "Elapsed: " << ms_int.count() << "ms" << std::endl;\
    }

struct BenchmarkPart1 : public noa::utils::combine::MakeDynamic<BenchmarkPart1> {
    using Depends = noa::utils::combine::Nodeps;

    unsigned int input = 0;

    static constexpr std::size_t bufSize = std::size_t{1} << 10;

    int buffer[bufSize];

    void run(const noa::utils::combine::CComputation auto& comp) {
        std::srand(input);
        for (std::size_t i = 0; i < bufSize; ++i) buffer[i] = std::rand();
    } // <-- BenchmarkBody::run

    long sumNear(std::size_t i) const {
        long ret = 0;
        const std::size_t min = (i > 2) ? (i - 3) : 0;
        const std::size_t max = (i < bufSize - 2) ? (i + 3) : bufSize;
        for (std::size_t j = min; j < max; ++j) {
            ret += buffer[j];
        }
        return ret;
    }
}; // <-- struct BenchmarkPart1

/// \brief Combine benchmark body
struct BenchmarkBody : public noa::utils::combine::MakeDynamic<BenchmarkBody> {
    using Depends = noa::utils::combine::DependencyList<BenchmarkPart1>;

    long result;

    void run(const noa::utils::combine::CComputation auto& comp) {
        result = 0;
        const auto& pt1 = comp.template get<BenchmarkPart1>();
        for (std::size_t i = 0; i < BenchmarkPart1::bufSize; ++i) {
            result += pt1.sumNear(i);
        }
    } // <-- BenchmarkBody::run
}; // <-- struct BenchmarkBody
