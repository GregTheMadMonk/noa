// Standard library
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>

#include "bench.hh"

using noa::utils::combine::DependencyList;
using noa::utils::combine::Nodeps;
using noa::utils::combine::MakeDynamic;
using noa::utils::combine::CComputation;

struct Task1 : public MakeDynamic<Task1> {
    using Depends = Nodeps;

    int input;

    void run(const CComputation auto& comp) {
        // std::cout << "Task1 run()" << std::endl;
    }
};

struct Task2 : public MakeDynamic<Task2> {
    using Depends = DependencyList<Task1>;

    int result;

    void run(const CComputation auto& comp) {
        // std::cout << "Task2 run()" << std::endl;
        result = comp.template get<Task1>().input * 2;
    }
};

struct Task3 : public MakeDynamic<Task3> {
    using Depends = DependencyList<Task1>;

    int result;

    void run(const CComputation auto& comp) {
        // std::cout << "Task3 run()" << std::endl;
        const auto& task1 = comp.template get<Task1>();
        result = task1.input * task1.input;
    }
};

struct Task4 : public MakeDynamic<Task4> {
    using Depends = DependencyList<Task2, Task3>;

    int result;

    void run(const CComputation auto& comp) {
        // std::cout << "Task4 run()" << std::endl;
        result = comp.template get<Task2>().result + comp.template get<Task3>().result;
    }
};

constexpr std::size_t runs = std::size_t{1} << 25;

// Entry point
int main(int argc, char** argv) {
    std::cout << "\'Combine\' (NOA task manager) functional tests." << std::endl;

    std::cout << "StaticComputation:" << std::endl;
    using noa::utils::combine::StaticComputation;
    StaticComputation<Task4> comp;
    comp.template get<Task1>().input = 3;

    BMARK([&comp] {
        for (std::size_t i = 0; i < runs; ++i) {
            comp.run();
        }
        std::cout << "Result: input * 2 + input * input = " << comp.template get<Task4>().result << std::endl;
    });

    std::cout << "DynamicComputation:" << std::endl;
    using noa::utils::combine::DynamicComputation;
    DynamicComputation dcomp;
    const std::size_t task4Index = Task4::index(); // No `constexpr`! Fully run-time
    dcomp.setTasks({ task4Index });
    dcomp.update();
    dcomp.template get<Task1>().input = 3;

    BMARK([&dcomp] {
        for (std::size_t i = 0; i < runs; ++i) {
            dcomp.run();
        }
        std::cout << "Result: input * 2 + input * input = " << dcomp.template get<Task4>().result << std::endl;
    });

    unsigned int seed = std::time(nullptr);
    std::cout << "Random initializer: " << seed << std::endl;

    std::cout << "Static:" << std::endl;
    StaticComputation<BenchmarkBody> bcomp;
    BMARK(bcomp.run);

    DynamicComputation bdcomp;
    bdcomp.template setTasks<BenchmarkBody>();
    bdcomp.update();

    std::cout << "Dynamic:" << std::endl;
    BMARK(bdcomp.run);

    return 0;
}
