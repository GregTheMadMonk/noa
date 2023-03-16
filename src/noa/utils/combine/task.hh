/**
 * \file task.hh
 * \brief Combine static task traits
 *
 * This file defines compile-time type utilities for workking
 * with static tasks.
 */

#pragma once

#include <type_traits>

#include "dummies.hh"
#include "template_details.hh"

namespace noa::utils::combine {

namespace detail {

    /// \brief Determines if a class has a `Depends` subtype that is a \ref DependencyList
    ///
    /// TODO C++20: Replace SIFNAE with a concept
    template <typename Task, typename = void>
    struct HasDepends {
        using Type = std::false_type;
    };

    template <typename Task>
    struct HasDepends<
        Task,
        std::enable_if_t<
            isDependencyList<typename Task::Depends>
        >
    > {
        using Type = std::true_type;
    };

} // <-- namespace detail

/// \brief Does the class have a `Depends` subtype that is a \ref DependencyList
///
/// Either `std::true_type` or `std::false_type`
///
/// TODO C++20: Replace with a concept
template <typename Task>
using HasDepends = typename detail::HasDepends<Task>::Type;

/// \brief An alias for `HasDepends<Task>::value`
///
/// TODO C++20: Replace with a concept
template <typename Task>
constexpr bool hasDepends = HasDepends<Task>::value;

/// \brief Determines whether the class has a non-static `run` method that accepts
/// a computation as an argument
///
/// TODO C++20: Replace with a concept
template <typename Task, typename = void>
constexpr bool hasRun = false;

template <typename Task>
constexpr bool hasRun<
                    Task,
                    std::enable_if_t<
                        std::is_same_v<
                            decltype(Task().run(detail::DummyComputation()), int()),
                            int
                        >
                    >
> = true;

/// \brief Deterimies whether a type is a valid task
template <typename Task>
constexpr bool isTask = hasDepends<Task> && hasRun<Task>;

} // <-- namespace noa::utils::combine
