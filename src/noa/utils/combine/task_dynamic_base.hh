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
        virtual void runDynamic(const DynamicComputation&) = 0;

        /// \brief Type index getter
        virtual std::size_t type() const noexcept = 0;

        /// \brief Get dependencies indices
        virtual std::vector<std::size_t> depends() const noexcept = 0;

        virtual ~DynamicTaskBase() = default;
    }; // <-- struct DynamicTaskBase

    /// \brief Task factory base class
    ///
    /// Used for creating tasks from type indices
    struct TaskFactoryBase {
        /// \brief Create the task
        virtual std::unique_ptr<DynamicTaskBase> create() const = 0;

        virtual ~TaskFactoryBase() = default;
    }; // <-- class TaskFactoryBase

    using TaskFactoryPtr = std::unique_ptr<TaskFactoryBase>;

    /// \brief Specific task factory
    template <typename Task>
    struct TaskFactory : TaskFactoryBase {
        std::unique_ptr<DynamicTaskBase> create() const override {
            return std::unique_ptr<DynamicTaskBase>{dynamic_cast<DynamicTaskBase*>(new Task())};
        } // <-- create()
    }; // <-- class TaskFactory

    /// \brief Unique task index generator
    class DynamicTaskIndexer {
        /// \brief Next index to dispatch
        inline static std::size_t val = 0;

        /// \brief Task factory
        inline static std::unordered_map<std::size_t, TaskFactoryPtr> factories;
        public:
        /// \brief Current index
        std::size_t index;

        /// \brief Default constructor
        template <typename Task>
        DynamicTaskIndexer(Task* dummy) : index(++val) {
            factories[index] = std::make_unique<TaskFactory<Task>>();
        }

        /// \brief Create a task of a cerain type
        static std::unique_ptr<DynamicTaskBase> createTask(std::size_t type) {
            return factories.at(type)->create();
        } // <-- createTask()
    }; // <-- class DynamicTaskIndexer

} // <-- namespace detail

} // <-- namespace noa::utils::combine
