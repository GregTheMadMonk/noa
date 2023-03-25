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

namespace concepts_detail {

    template <template <typename...> class ComputationCandidate>
    requires requires (ComputationCandidate<detail::DummyTask> cc) {
        { cc.template get<detail::DummyTask>() } -> std::same_as<detail::DummyTask&>;
    } constexpr bool hasGet<ComputationCandidate> = true;

    template <template <typename...> class ComputationCandidate>
    requires requires (const ComputationCandidate<detail::DummyTask> cc) {
        { cc.template get<detail::DummyTask>() } -> std::same_as<const detail::DummyTask&>;
    } constexpr bool hasConstGet<ComputationCandidate> = true;

} // <-- namespace concepts_detail

} // <-- namespace noa::utils::combine
