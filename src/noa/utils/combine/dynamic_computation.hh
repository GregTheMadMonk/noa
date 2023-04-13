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
 * \file dynamic_computation.hh
 * \brief Combine's dynamic computation definition
 */

#pragma once

#include <algorithm>
#include <memory>
#include <unordered_map>
#include <vector>

#include "concepts_prelude.hh"
#include "task_dynamic_base.hh"

namespace noa::utils::combine {

namespace detail {

    /**
     * \brief Allows for the tasks list to be manipulated at runtime
     *
     * A dummy list of template parameters exists for compatibility. Use \ref DynamicComputation
     */
    template <typename... Dummy>
    class DynamicComputationT {
        /// \brief Unique pointer to a task
        using TaskPtr = std::unique_ptr<DynamicTaskBase>;
        /// \brief Tasks list
        std::vector<TaskPtr> tasks; 

        /// \brief Task lookup map
        std::unordered_map<std::size_t, std::size_t> taskMap;

        /// \brief Tasks that are user-required
        std::vector<std::size_t> input;

        public:
        /// \brief Set required end tasks from type indices
        void setTasks(const std::vector<std::size_t>& taskTypes) {
            input = taskTypes;
        } // <-- setTasks(taskTypes)

        /// \brief Set required end tasks from template parameter pack
        template <CTask... Tasks>
        void setTasks() {
            input.clear();
            (input.push_back(Tasks::index()), ...);
        } // <-- setTasks()

        /// \brief Update task queue with input
        void update() {
            this->tasks.clear();

            std::vector<std::size_t> typesOrder;
            std::copy(input.begin(), input.end(), std::back_inserter(typesOrder));

            std::size_t offset = 1;
            while (offset != typesOrder.size() + 1) {
                const std::size_t index = typesOrder.size() - offset;
                const auto depends = detail::DynamicTaskIndexer::getDependencies(typesOrder.at(index));

                // If the new dependency is alrady present in the list, move it to the start
                for (std::size_t newTask : depends) {
                    const auto it = std::find(typesOrder.begin(), typesOrder.end(), newTask);

                    if (it != typesOrder.end()) {
                        auto shifter = it;
                        while (shifter != typesOrder.begin()) {
                            std::swap(*shifter, *std::prev(shifter));
                            --shifter;
                        }
                    } else {
                        // Else insert to the front
                        typesOrder.insert(typesOrder.begin(), newTask);
                    }
                }

                ++offset;
            }

            // Initialize tasks and populate the map
            this->taskMap.clear();
            for (std::size_t type : typesOrder) {
                this->taskMap[type] = this->tasks.size();
                this->tasks.push_back(detail::DynamicTaskIndexer::createTask(type, *this));
            }
        } // <-- update()

        /// \brief Run computation
        void run() {
            for (auto& task : this->tasks) {
                task->runDynamic(*this);
            }
        } // <-- void run()

        /// \brief Gets a task of requested type dynamically from the tasks list
        template <typename Task>
        Task& get() {
            const auto& index = this->taskMap.at(Task::index());
            return *dynamic_cast<Task*>(this->tasks[index].get());
        } // <-- Task& get()

        /// \brief Gets a task of requested type dynamically from the tasks list (const overload)
        template <typename Task>
        const Task& get() const {
            const auto& index = this->taskMap.at(Task::index());
            return *dynamic_cast<Task*>(this->tasks[index].get());
        } // <-- const Task& get() const
    }; // <-- class DynamicComputationT

} // <-- namespace detail

using DynamicComputation = detail::DynamicComputationT<>;

} // <-- namespace noa::utils::combine
