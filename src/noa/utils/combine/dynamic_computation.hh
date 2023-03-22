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
        template <typename... Tasks>
        void setTasks() {
            static_assert((isTask<Tasks> && ... && true), "All template parameters must be valid tasks!");

            input.clear();
            (input.push_back(Tasks::index()), ...);
        } // <-- setTasks()

        /// \brief Update task queue with input
        void update() {
            this->tasks.clear();

            for (std::size_t type : input) {
                this->tasks.push_back(detail::DynamicTaskIndexer::createTask(type));
            }

            std::size_t offset = 1;
            while (offset != this->tasks.size() + 1) {
                const std::size_t index = this->tasks.size() - offset;
                const auto depends = this->tasks.at(index)->depends();

                for (std::size_t newTask : depends) {
                    const auto it = std::find_if(
                        this->tasks.begin(),
                        this->tasks.end(),
                        [newTask] (const auto& tp) { return tp->type() == newTask; }
                    );

                    // If the new dependecny is already present in the list, move it to the start
                    if (it != this->tasks.end()) {
                        // TODO: C++20 shift_right
                        auto shifter = it;
                        while (shifter != this->tasks.begin()) {
                            std::swap(*shifter, *std::prev(shifter));
                            --shifter;
                        }
                    } else {
                        // Else insert to the front
                        this->tasks.insert(this->tasks.begin(), detail::DynamicTaskIndexer::createTask(newTask));
                    }
                }

                offset++;
            }

            // Populate task map
            this->taskMap.clear();
            for (std::size_t i = 0; i < this->tasks.size(); ++i) {
                this->taskMap[this->tasks.at(i)->type()] = i;
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
