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
 * \file task.hh
 * \brief Combine static task traits
 *
 * This file defines compile-time type utilities for workking
 * with static tasks.
 */

#pragma once

#include <type_traits>

#include "concepts_prelude.hh"
#include "dummies.hh"
#include "template_details.hh"

namespace noa::utils::combine {

namespace concepts_detail {

    template <std::constructible_from<detail::DummyComputation<>&> Task>
    constexpr bool constructibleFromComputation<Task> = true;

} // <-- namespace concepts_detail

/// \brief Task dependency list
template <CTask Task>
using GetDependencies = meta::VTCast<DependencyList, meta::VTRemoveCVR<meta::GetArgTypes<&Task::run>>>;

template <typename Task, CComputation Computation>
struct TaskInvoker {
    void invoke(Task& task, Computation comp) {
    }
};

/// \brief Invoke task and pass required dependencies
template <typename Task, CComputation Computation>
void invokeTask(Task& task, Computation& comp) {
    static constexpr auto captureDependencies = [] <CTask... Tasks> (Computation& comp, DependencyList<Tasks...>) -> std::tuple<Tasks&...> {
        return std::tie(comp.template get<Tasks>()...);
    };

    std::apply(
        [&task] (auto& ... tasks) { task.run(tasks...); },
        captureDependencies(comp, GetDependencies<Task>{})
    );
}

} // <-- namespace noa::utils::combine
