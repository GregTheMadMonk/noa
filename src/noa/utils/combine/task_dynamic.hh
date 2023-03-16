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
