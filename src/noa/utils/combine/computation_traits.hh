/*****************************************************************************
 *   Copyright (c) 2022, Roland Grinis, GrinisRIT ltd.                       *
 *   (roland.grinis@grinisrit.com)                                           *
 *   All rights reserved.                                                    *
 *   See the file COPYING for full copying permissions.                      *
 *                                                                           *
 *   This program is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation, either version 3 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the            *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 *                                                                           *
 *   Implemented by: Gregory Dushkin (yagreg7@gmail.com)                     *
 *****************************************************************************/
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
