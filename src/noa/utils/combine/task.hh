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
#include <typeinfo>

#include "concepts_prelude.hh"
#include "dummies.hh"
#include "template_details.hh"

namespace noa::utils::combine {

/// \brief Get dependencies generated from task's `run` method
template <CTask Task>
using GetRunDependencies = meta::VTCast<
    DependencyList,
    meta::VTRemoveCVR<meta::GetArgTypes<&Task::run>>
>;

/// \brief Get dependencies generated from a task's constructor
template <CTask Task>
using GetConstructorDependencies = meta::VTCast<
    DependencyList,
    meta::VTRemoveCVR<meta::GetConstructorArgTypes<Task>>
>;

/// \brief Task dependency list
template <CTask Task>
using GetDependencies = DepListsJoin<
    GetConstructorDependencies<Task>,
    GetRunDependencies<Task>
>;

namespace detail {
    template <CComputation Computation, CTask... Tasks>
    std::tuple<Tasks&...> captureDependencies(Computation& comp, DependencyList<Tasks...>) {
        return std::tie(comp.template get<Tasks>()...);
    };
}

/// \brief Construct task and pass required dependencies into its constructor
template <CTask Task, CComputation Computation>
Task constructTask(Computation& comp) {
    return std::apply(
        [] (auto& ... tasks) { return Task(tasks...); },
        detail::captureDependencies(comp, GetConstructorDependencies<Task>{})
    );
} // <-- constructTask()

/// \brief Construct task via `new` and pass required dependencies into its constructor
template <CTask Task, CComputation Computation>
Task* newTask(Computation& comp) {
    return std::move(
        std::apply(
            [] (auto& ... tasks) { return new Task(tasks...); },
            detail::captureDependencies(comp, GetConstructorDependencies<Task>{})
        )
    );
} // <-- newTask()

/// \brief Invoke task and pass required dependencies
template <CTask Task, CComputation Computation>
void invokeTask(Task& task, Computation& comp) {
    std::apply(
        [&task] (auto& ... tasks) { task.run(tasks...); },
        detail::captureDependencies(comp, GetRunDependencies<Task>{})
    );
} // <-- invokeTask()

} // <-- namespace noa::utils::combine
