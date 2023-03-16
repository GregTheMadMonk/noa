/**
 * \file dynamic_computation.hh
 * \brief Combine's dynamic computation definition
 */

#pragma once

#include <algorithm>
#include <memory>
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
            const auto it = std::find_if(
                tasks.begin(),
                tasks.end(),
                [] (const TaskPtr& tp) {
                    return tp->type() == Task::index();
                }
            );

            if (it == tasks.end()) {
                throw std::runtime_error("Could not find the task!");
            }

            return *dynamic_cast<Task*>(it->get());
        } // <-- Task& get()

        /// \brief Gets a task of requested type dynamically from the tasks list (const overload)
        template <typename Task>
        const Task& get() const {
            const auto it = std::find_if(
                tasks.begin(),
                tasks.end(),
                [] (const TaskPtr& tp) {
                    return tp->type() == Task::index();
                }
            );

            if (it == tasks.end()) {
                throw std::runtime_error("Could not find the task!");
            }

            return *dynamic_cast<Task*>(it->get());
        } // <-- const Task& get() const
    }; // <-- class DynamicComputationT

} // <-- namespace detail

using DynamicComputation = detail::DynamicComputationT<>;

} // <-- namespace noa::utils::combine
