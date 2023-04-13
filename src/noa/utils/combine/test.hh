/**
 * \file test.hh
 * \brief Static testing of Combine components
 */

#pragma once

// Standard headers
#include <type_traits>

// Local headers
// No need in them since this file is included in combine.hh which already includes
// all other Combine headers before including this file

//// \brief Namespace for dummies required for static testing of Combine components
namespace noa::utils::combine::test {

// A dummy template
template <typename...> struct Dummy1 {};

// Does not satisfy any criteria to be a valid task
struct NotATask {};
// Has a proper dependency list suntype but nothing else
struct HasDependsSubtype {
    using Depends = Nodeps;
};
// Has a `Depends` subtype which is not an instance of `DependencyList`
struct HasWrongDependsSubtype {
    using Depends = Dummy1<>;
};
// Have a `run` method with a wrong signature
struct HasWrongRunSignature1 {
    void run();
};
struct HasWrongRunSignature2 {
    int run(const CComputation auto&);
};

// Testing tasks
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

// Test LastVArg
static_assert(
    std::is_same_v<
        LastVArg<Dummy1<int, char, float>>,
        float
    >, "LastVArg test failed"
);

// Test VConvert
static_assert(
    std::is_same_v<
        VConvert<detail::DummyT, Dummy1<int, char, float>>,
        detail::DummyT<int, char, float>
    >, "VConvert test failed"
);

// Test concepts
// Test CHasDepends
static_assert(
    !CHasDepends<NotATask>,
    "CHasDepends: Task without a dependency list reported to have one"
);
static_assert(
    CHasDepends<HasDependsSubtype>,
    "CHasDepends: dependency list not detected on a type"
);
static_assert(
    !CHasDepends<HasWrongDependsSubtype>,
    "CHasDepends: dependency list type mismatch undetected"
);
static_assert(
    CHasDepends<detail::DummyTask>,
    "CHasDepends: DummyTask's valid `Depends` subtype undetected"
);

// Test CHasRun
static_assert(
    !CHasRun<HasDependsSubtype>,
    "CHasRun: true on a type without a `run` method"
);
static_assert(
    !CHasRun<HasWrongRunSignature1>,
    "CHasRun: true on a type with a wrong `run` method signature (no agument)"
);
static_assert(
    !CHasRun<HasWrongRunSignature2>,
    "CHasRun: true on a type with a wrong `run` method signature (non-void return type)"
);
static_assert(
    CHasRun<detail::DummyTask>,
    "CHasRun: DummyTask's `run` method is not recognized"
);

// Test CTask
static_assert(
    CTask<detail::DummyTask>,
    "CTask: DummyTask is not detected as a valid task"
);

// DependencyList tests
// DependencyList `empty` member
static_assert(
    DependencyList<>::empty,
    "Empty DependencyList `empty` member evaluates to `false`"
);
static_assert(
    !DependencyList<Task1, Task2>::empty,
    "Non-empty DependencyList `empty` member evaluates to `true`"
);
// Joining dependency lists
static_assert(
    std::is_same_v<
        DepListsJoin<
            DependencyList<Task1>,
            DependencyList<Task2, Task3>
        >,
        DependencyList<Task1, Task2, Task3>
    >, "Two `DependencyList`s joining faled"
);
static_assert(
    std::is_same_v<
        DepListsJoin<
            DependencyList<Task1>,
            DependencyList<Task2, Task3>,
            DependencyList<Task1, Task2, Task3>
        >,
        DependencyList<Task1, Task2, Task3, Task1, Task2, Task3>
    >, "Three `DependencyList`s joining failed"
);
// Removing duplicates from a `DependencyList`
static_assert(
    std::is_same_v<
        DependencyList<Task1, Task2, Task2, Task2, Task3, Task2, Task3, Task1, Task2>::Uniquify,
        DependencyList<Task1, Task2, Task3>
    >, "DepndencyList::Uniquify: Incorrect result"
);

// Check computation concepts
// DummyComputation
static_assert(
    CHasGet<detail::DummyComputation>,
    "`DummyComputation`'s `get` method undetected"
);
static_assert(
    CHasConstGet<detail::DummyComputation>,
    "`DummyComputation`'s const `get` method undetected"
);
static_assert(
    CComputationTemplate<detail::DummyComputation>,
    "`DummyComputation` is not detected as a valid computation template"
);

// StaticComputation
static_assert(
    CHasGet<StaticComputation>,
    "`StaticComputation`'s `get` method undetected"
);
static_assert(
    CHasConstGet<StaticComputation>,
    "`StaticComputation`'s const `get` method undetected"
);
static_assert(
    CComputationTemplate<StaticComputation>,
    "`StaticComputation` is not detected as a valid computation template"
);

// DynamicComputation
static_assert(
    CHasGet<detail::DynamicComputationT>,
    "`DynamicComputationT`'s `get` method undetected"
);
static_assert(
    CHasConstGet<detail::DynamicComputationT>,
    "`DynamicComputationT`'s const `get` method undetected"
);
static_assert(
    CComputationTemplate<detail::DynamicComputationT>,
    "`DynamicComputationT` is not detected as a valid computation template"
);

// Test static computation specifically
static_assert(
    std::is_same_v<
        typename StaticComputation<Task1, Task2, Task3>::TasksOrder,
        DependencyList<Task1, Task2, Task3>
    >, "StaticComputation: Failed tasks ordering (no dependencies)"
);

static_assert(
    std::is_same_v<
        typename StaticComputation<Task4, Task5>::TasksOrder,
        DependencyList<Task1, Task2, Task4, Task5>
    >, "StaticComputation: Failed tasks ordering with one dependency level"
);

static_assert(
    std::is_same_v<
        typename StaticComputation<Task4, Task5, Task1>::TasksOrder,
        DependencyList<Task1, Task2, Task4, Task5>
    >, "StaticComputation: Failed tasks ordering (one dependency level + duplicate tasks)"
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
    >, "StaticComputation: Failed tasks ordering (two-level dependency)"
);

} // <-- namespace noa::utils::combine::test
