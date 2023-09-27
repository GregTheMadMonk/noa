/**
 * \file task_manip.hh
 * \brief Constructing and running Combine tasks
 */
#pragma once

#include <optional>
#include <memory>
#include <vector>

#include "task_traits.hh"

namespace noa::combine::task_manip {

namespace detail {
    struct Dummy { void run(); };
} // <-- namespace detail

/// \brief A primitive validity checker. Could break easily
template <typename C>
concept Composer = requires (C c) {
    c.getList(utils::meta::List{});
    c.template get<detail::Dummy>();
};

/// \brief Emplace a task in std::optional
template <Task TaskType, typename... Args>
void emplace(std::optional<TaskType>& opt, Args&&... args) {
    opt.emplace(std::forward<Args>(args)...);
} // <-- void emplace(std::optional<TaskType>, args...)

/// \brief Construct the task from the composer
template <Task TaskType>
constexpr inline void constructTask(
    auto& task,
    auto& composer,
    auto&&... initializers
) {
    using ArgList = combine::detail::ConstructorDeps<TaskType>;
    std::apply(
        [&task] (auto&... deps) {
            emplace<TaskType>(task, deps...);
        }, composer.getList(ArgList{})
    );

    // Apply initializers
    (
        [&f=initializers, &task] {
            if constexpr (std::invocable<decltype(f), TaskType&>) {
                f(*task);
            }
        } (), ...
    );
} // <-- void constructTask(task, comp, initializers...)

/// \brief Copy-construct the task from the composer
template <Task TaskType, Composer ComposerType>
constexpr inline void copyConstructTask(
    auto& task,
    ComposerType& composer,
    const ComposerType& other
) {
    using ArgList = combine::detail::CopyDeps<TaskType>;
    std::apply(
        [&task, &other] (auto&... deps) {
            emplace<TaskType>(
                task,
                TaskCopy{}, other.template get<TaskType>(), deps...
            );
        }, composer.getList(ArgList{})
    );
} // <-- void copyConstructTask(task, composer, other)

/// \brief Move-construct the task for the composer
template <Task TaskType, Composer ComposerType>
constexpr inline void moveConstructTask(
    auto& task,
    ComposerType& composer,
    ComposerType& other
) {
    using ArgList = combine::detail::MoveDeps<TaskType>;
    std::apply(
        [&task, &other] (auto&... deps) {
            emplace<TaskType>(
                task,
                TaskMove{},
                std::move(other.template get<TaskType>()), deps...
            );
        }, composer.getList(ArgList{})
    );
} // <-- void moveConstructTask(task, composer, other)

/// \brief Run the task from the composer
template <Task TaskType, Composer ComposerType>
constexpr inline void runTask(TaskType& task, ComposerType& composer) {
    using ArgList = combine::detail::RunDeps<TaskType>;
    std::apply(
        [&task] (auto&... deps) { task.run(deps...); },
        composer.getList(ArgList{})
    );
} // <-- void runTask(task)

} // <-- namespace noa::combine::task_manip
