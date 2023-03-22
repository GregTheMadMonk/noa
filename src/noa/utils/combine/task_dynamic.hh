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
 * \file task_dynamic.hh
 * \brief Dynamic dispatch support for tasks
 */

#pragma once

#include "dependency_list.hh"
#include "task_dynamic_base.hh"

namespace noa::utils::combine {

/// \brief Provide dynamic dispatch support for task Task
///
/// To allow usage with \ref DynamicComputation, a `Task` task type
/// should inherit from `MakeDynamic<Task>`
template <typename Task>
class MakeDynamic : public detail::DynamicTaskBase {
    /// \brief Unique task type index generator
    inline static detail::DynamicTaskIndexer indexer{static_cast<Task*>(nullptr)};

    template <typename... Tasks>
    static std::vector<std::size_t> dependencyHelper(DependencyList<Tasks...>) {
        std::vector<std::size_t> ret;

        (ret.push_back(Tasks::index()), ...);

        return ret;
    }

    public:
    // TODO: Figure out a way to check that `Task` is a valid task.
    // Can't now because `Task` is an incomplete type at the moment
    // of instantiation

    /// \brief Calls a `run` method on itself
    void runDynamic(const DynamicComputation& comp) override {
        dynamic_cast<Task*>(this)->run(comp);
    } // <-- void runDynamic()

    /// \brief Get type index from type name
    static std::size_t index() noexcept { return indexer.index; }

    /// \brief Get type index from a dynamically polymorphic reference/pointer
    std::size_t type() const noexcept override { return indexer.index; }

    /// \brief Get dependencies type indices
    std::vector<std::size_t> depends() const noexcept override {
        return dependencyHelper(typename Task::Depends{});
    } // <-- depends()
}; // <-- class MakeDynamic

} // <-- namespace noa::utils::combine
