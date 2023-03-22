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
 * \file static_computation.hh
 * \brief Combine's static computation definition
 *
 * See \ref StaticComputation
 */

#pragma once

#include <tuple>

#include "dummies.hh"

namespace noa::utils::combine {

namespace detail {

    /// \brief Unravel tasks list with dependencies
    template <typename List1, typename... Lists> struct Unravel {
        using Type = typename Unravel<typename List1::Depends, List1, Lists...>::Type;
    };

    template <typename... Lists> struct Unravel<DependencyList<>, Lists...> {
        using Type = typename DepListsJoin<Lists...>::Uniquify;
    };

} // <-- namespace detail

/// \brief A static computation struct
///
/// `Static` refers to a task dependencies resolution method
/// rather than the actual computation. Order of execution of
/// tasks by this class is determined at compile-time and cannot
/// be changed.
///
/// \tparam Tasks tasks to be processed. Tasks are ordered as they follow in the list, left-to-right
template <typename... Tasks>
struct StaticComputation {
    static_assert(
        (isTask<Tasks> && ... && true),
        "All StaticComputation template arguments are required to be valid tasks"
    );

    using TasksOrder = typename detail::Unravel<DependencyList<Tasks...>>::Type;

    private:
    /// \brief Task states are stored in a tuple
    using State = VConvert<std::tuple, TasksOrder>;
    /// \brief Task states
    State state{};

    public:
    /// \brief Get a reference to a task state
    template <typename Task>
    Task& get() { return std::get<Task>(this->state); }

    /// \brief Const-overload for the get() function
    template <typename Task>
    const Task& get() const { return std::get<Task>(this->state); }

    /// \brief Run tasks sequentially
    void run() {
        std::apply(
            [this] (auto&& ... tasks) {
                (tasks.run(*this), ...);
            }, this->state
        );
    } // <-- void run()
}; // <-- struct StaticComputation

} // <-- noa::utils::combine
