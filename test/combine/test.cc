// Standard library
#include <iostream>

#include "static_test.hh"

using noa::utils::combine::DependencyList;
using noa::utils::combine::Nodeps;

struct Task1 {
    using Depends = Nodeps;

    int input;

    template <typename Computation>
    void run(const Computation& comp) {}
};

struct Task2 {
    using Depends = DependencyList<Task1>;

    int result;

    template <typename Computation>
    void run(const Computation& comp) {
        result = comp.template get<Task1>().input * 2;
    }
};

struct Task3 {
    using Depends = DependencyList<Task1>;

    int result;

    template <typename Computation>
    void run(const Computation& comp) {
        const auto& task1 = comp.template get<Task1>();
        result = task1.input * task1.input;
    }
};

struct Task4 {
    using Depends = DependencyList<Task2, Task3>;

    int result;

    template <typename Computation>
    void run(const Computation& comp) {
        result = comp.template get<Task2>().result + comp.template get<Task3>().result;
    }
};

// Entry point
int main(int argc, char** argv) {
    std::cout << "\'Combine\' (NOA task manager) functional tests." << std::endl;

    using noa::utils::combine::StaticComputation;
    StaticComputation<Task4> comp;
    comp.template get<Task1>().input = 3;
    comp.run();

    std::cout << "Result: input * 2 + input * input = " << comp.template get<Task4>().result << std::endl;

    return 0;
}
