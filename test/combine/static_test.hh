/**
 * \file static_test.hh
 * \brief A header for compile-time tests of Combine task manager
 */

#pragma once

#include <noa/utils/combine/combine.hh>

namespace noa::utils::combine::test {

template <typename...> struct Dummy1 {};

struct NotATask {};

struct HasDependsSubtype {
    using Depends = DependencyList<>;
};

struct RunWrongSignature {
    void run();
};

struct HasWrongDependsSubtype {
    using Depends = Dummy1<int>;
};

struct Task1 : detail::DummyTask {};
struct Task2 : detail::DummyTask {};
struct Task3 : detail::DummyTask {};

struct Task4 : detail::DummyTask {
    using Depends = DependencyList<Task1>;
};

struct Task5 : detail::DummyTask {
    using Depends = DependencyList<Task2>;
};

struct Task6 : detail::DummyTask {
    using Depends = DependencyList<Task1, Task3, Task5>;
};

static_assert(
    std::is_same_v<
        VConvert<detail::DummyT, Dummy1<int, char, float>>,
        detail::DummyT<int, char, float>
    >, "VConvert fail"
);

static_assert(std::is_same_v<LastVArg<Dummy1<int, char, float>>, float>);

static_assert(!hasDepends<NotATask>,                "Task without a dependency list reported to have one");
static_assert(hasDepends<HasDependsSubtype>,        "Task dependency list not detected");
static_assert(!hasDepends<HasWrongDependsSubtype>,  "Task with a wrong dependency list type undetected!");

static_assert(!hasRun<HasDependsSubtype>,           "Undetected lack of non-statc `run` method");
static_assert(!hasRun<RunWrongSignature>,           "Wrong `run` method signature undetected!");

static_assert(hasDepends<detail::DummyTask>, "DummyTask does not provide a dependency list!");
static_assert(hasRun<detail::DummyTask>,     "DummyTask does not provide a non-static run() method!");
static_assert(isTask<detail::DummyTask>,     "DummyTask does not qualify for a Task!");

static_assert(DependencyList<>::empty, "Empty DependencyList `empty` member unset");
static_assert(DependencyList<Task1, Task2>::Depends::empty, "Empty DependencyList dependency undetected");

static_assert(
    std::is_same_v<
        typename DependencyList<Task4, Task5>::Depends,
        DependencyList<Task1, Task2>
    >, "DependencyList next dependency level failed"
);

static_assert(
    std::is_same_v<
        DepListsJoin<DependencyList<Task1>, DependencyList<Task2, Task3>>,
        DependencyList<Task1, Task2, Task3>
    >, "Two dependency lists join failed"
);

static_assert(
    std::is_same_v<
        DepListsJoin<DependencyList<Task1>, DependencyList<Task2, Task3>, DependencyList<Task1, Task2, Task3>>,
        DependencyList<Task1, Task2, Task3, Task1, Task2, Task3>
    >, "Three dependency lists join failed"
);

static_assert(
    std::is_same_v<
        DependencyList<Task1, Task2, Task2, Task2, Task3, Task2, Task3, Task1, Task2>::Uniquify,
        DependencyList<Task1, Task2, Task3>
    >, "DependencyList::Uniquify: Incorrect result"
);

static_assert(compHasGet<detail::DummyComputation>, "DummyComputation doesn't have a proper `get` method");
static_assert(compHasConstGet<detail::DummyComputation>, "DummyComputation doesn't have a proper const `get` method");
static_assert(isComputation<detail::DummyComputation>, "DummyComputation is not detected as a valid computation");

static_assert(
    std::is_same_v<
        typename StaticComputation<Task1, Task2, Task3>::TasksOrder,
        DependencyList<Task1, Task2, Task3>
    >, "StaticComputation: failed without dependencies"
);

static_assert(
    std::is_same_v<
        typename StaticComputation<Task4, Task5>::TasksOrder,
        DependencyList<Task1, Task2, Task4, Task5>
    >, "StaticComputation: failed with one dependency level"
);

static_assert(
    std::is_same_v<
        typename StaticComputation<Task4, Task5, Task1>::TasksOrder,
        DependencyList<Task1, Task2, Task4, Task5>
    >, "StaticComputation: failed with one dependency level + duplicate tasks"
);

/*
 * 6 -  4
 * |\ \ |
 * 5 3 -1
 * |
 * 2
 *
 * Task list is formed from bottom to top:
 * 2 -> 1-3-5 -> 4-6
 */
static_assert(
    std::is_same_v<
        typename StaticComputation<Task4, Task6>::TasksOrder,
        DependencyList<Task2, Task1, Task3, Task5, Task4, Task6>
    >, "StaticComputation: two-level dependency unravel failed"
);

} // <-- namespace noa::utils::combine::test
