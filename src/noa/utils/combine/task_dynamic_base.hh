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
 * \file task_dynamic_base.hh
 * \brief Describes the base class fro dynamic tasks
 */

#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

namespace noa::utils::combine {

namespace detail {
    template <typename...> class DynamicComputationT;
}

using DynamicComputation = detail::DynamicComputationT<>;

namespace detail {

    /// \brief Base class for all dynamically-dispatched tasks
    struct DynamicTaskBase {
        /// \brief A virtual run() caller
        virtual void runDynamic(DynamicComputation&) = 0;

        /// \brief Type index getter
        virtual std::size_t type() const noexcept = 0;

        /// \brief Get dependencies indices
        virtual std::vector<std::size_t> depends() const noexcept = 0;

        virtual ~DynamicTaskBase() = default;
    }; // <-- struct DynamicTaskBase

    /// \brief Task type tools base class
    ///
    /// Provides tools for creation/handling of tasks of certain types via templated
    /// derived types
    struct TaskTypeToolsBase {
        /// \brief Create the task
        virtual std::unique_ptr<DynamicTaskBase> create(DynamicComputation& comp) const = 0;
        /// \brief Get task dependencies indices
        virtual std::vector<std::size_t> dependencies() const = 0;

        virtual ~TaskTypeToolsBase() = default;
    }; // <-- class TaskTypeToolsBase

    using TaskTypeToolsPtr = std::unique_ptr<TaskTypeToolsBase>;

    /// \brief Task type tools specialization
    template <CTask Task>
    struct TaskTypeTools : TaskTypeToolsBase {
        std::unique_ptr<DynamicTaskBase> create(DynamicComputation& comp) const override {
            return std::unique_ptr<DynamicTaskBase>{dynamic_cast<DynamicTaskBase*>(newTask<Task>(comp))};
        } // <-- create()

        std::vector<std::size_t> dependencies() const override {
            return Task::dependencies();
        }
    }; // <-- class TaskTypeTools

    /// \brief Unique task index generator
    class DynamicTaskIndexer {
        /// \brief Next index to dispatch
        inline static std::size_t val = 0;

        /// \brief Task factory
        inline static std::unordered_map<std::size_t, TaskTypeToolsPtr> factories;
        public:
        /// \brief Current index
        std::size_t index;

        /// \brief Default constructor
        template <typename Task>
        DynamicTaskIndexer(Task* dummy) : index(++val) {
            factories[index] = std::make_unique<TaskTypeTools<Task>>();
        }

        /// \brief Create a task of a cerain type
        static std::unique_ptr<DynamicTaskBase> createTask(std::size_t type, DynamicComputation& dynComp) {
            return factories.at(type)->create(dynComp);
        } // <-- createTask()

        /// \brief Get task dependencies
        static std::vector<std::size_t> getDependencies(std::size_t type) {
            return factories.at(type)->dependencies();
        }
    }; // <-- class DynamicTaskIndexer

} // <-- namespace detail

} // <-- namespace noa::utils::combine
