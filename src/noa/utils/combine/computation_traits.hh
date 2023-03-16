/**
 * \file computation_traits.hh
 * \brief Describes qualities that any Computation must satisfy
 */

#pragma once

#include <type_traits>

#include "dummies.hh"

namespace noa::utils::combine {

/// \brief Check for `get` method
template <template <typename...> class ComputationCandidate, typename = void>
constexpr bool compHasGet = false;

template <template <typename...> class ComputationCandidate>
constexpr bool
compHasGet<
    ComputationCandidate,
    std::enable_if_t<
        std::is_same_v<
            decltype(std::declval<ComputationCandidate<detail::DummyTask>>().template get<detail::DummyTask>()),
            detail::DummyTask&
        >
    >
> = true;

/// \brief Check for const `get` method
template <template <typename...> class ComputationCandidate, typename = void>
constexpr bool compHasConstGet = false;

template <template <typename...> class ComputationCandidate>
constexpr bool
compHasConstGet<
    ComputationCandidate,
    std::enable_if_t<
        std::is_same_v<
            decltype(
                std::declval<const ComputationCandidate<detail::DummyTask>>().template get<detail::DummyTask>()
            ), const detail::DummyTask&
        >
    >
> = true;

/// \brief Does the template type represent a proper computation?
template <template <typename...> class ComputationCandidate>
constexpr bool isComputation = compHasGet<ComputationCandidate> && compHasConstGet<ComputationCandidate>;

} // <-- namespace noa::utils::combine
