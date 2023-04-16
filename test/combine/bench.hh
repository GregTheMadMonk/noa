/**
 * \file bench.hh
 * \brief Primitive benchmarking for Combine tests
 */

// Standard library
#include <chrono>
#include <vector>

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
    unsigned int input = 0;

    static constexpr std::size_t bufSize = std::size_t{1} << 22;

    std::vector<int> buffer;

    BenchmarkPart1() {
        buffer = std::vector<int>(bufSize);
    }

    void run() {
        std::srand(input);
        for (auto& v : buffer) v = std::rand();
    } // <-- BenchmarkBody::run

    long sumNear(std::size_t i) const {
        long ret = 0;
        static constexpr auto window = std::size_t{1} << 8;
        const std::size_t min = (i > window - 1) ? (i - window) : 0;
        const std::size_t max = (i < bufSize - window + 1) ? (i + window) : bufSize;
        for (std::size_t j = min; j < max; ++j) {
            ret += buffer[j];
        }
        return ret;
    }
}; // <-- struct BenchmarkPart1

/// \brief Combine benchmark body
struct BenchmarkBody : public noa::utils::combine::MakeDynamic<BenchmarkBody> {
    long result;

    void run(BenchmarkPart1& pt1) {
        result = 0;

        const auto sumNear = [pt1] (std::size_t i) {
            long ret = 0;
            static constexpr auto window = std::size_t{1} << 13;
            const std::size_t min = (i > window - 1) ? (i - window) : 0;
            const std::size_t max = (i < pt1.bufSize - window + 1) ? (i + window) : pt1.bufSize;
            for (std::size_t j = min; j < max; ++j) {
                ret += pt1.buffer[j];
            }
            return ret;
        };

        for (std::size_t i = 0; i < BenchmarkPart1::bufSize; ++i) {
            result += sumNear(i);
        }
    } // <-- BenchmarkBody::run
}; // <-- struct BenchmarkBody
