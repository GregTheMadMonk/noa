/**
 * \file task_manip.hh
 * \brief Constructing and running Combine tasks
 */
#pragma once

#include <optional>
#include <memory>

#include "any_task.hh"
#include "task_traits.hh"

namespace noa::utils::combine::task_manip {

namespace detail {
    struct Dummy { void run(); };
} // <-- namespace detail

/// \brief A primitive validity checker. Could break easily
template <typename C>
concept Composer = requires (C c) {
    c.getList(meta::List{});
    c.template get<detail::Dummy>();
};

/// \brief Construct the task from the composer into std::optional
template <
    Task TaskType, Composer ComposerType, typename... Initializers
> constexpr inline void constructTask(
    std::optional<TaskType>& task,
    ComposerType& composer,
    Initializers&&... initializers
) {
    using ArgList = combine::detail::ConstructorDeps<TaskType>;
    std::apply(
        [&task] (auto&... deps) {
            task.emplace(deps...);
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

/// \brief Construct the task from the composer into \ref AnyTask
template <
    Task TaskType, Composer ComposerType, typename... Initializers
> constexpr inline void constructTask(
    AnyTask& task,
    ComposerType& composer,
    Initializers&&... initializers
) {
    using ArgList = combine::detail::ConstructorDeps<TaskType>;
    std::apply(
        [&task] (auto&... deps) {
            task.emplace<TaskType>(deps...);
        }, composer.getList(ArgList{})
    );

    // Apply initializers
    (
        [&f=initializers, &task] {
            if constexpr (std::invocable<decltype(f), TaskType&>) {
                f(task.get<TaskType>());
            }
        } (), ...
    );
} // <-- void constructTask(task, comp, initializers...)

/// \brief Copy-construct the task from the composer
template <Task TaskType, Composer ComposerType>
constexpr inline void copyConstructTask(
    std::optional<TaskType>& task,
    ComposerType& composer,
    const ComposerType& other
) {
    using ArgList = combine::detail::CopyDeps<TaskType>;
    std::apply(
        [&task, &other] (auto&... deps) {
            task.emplace(
                TaskCopy{}, other.template get<TaskType>(), deps...
            );
        }, composer.getList(ArgList{})
    );
} // <-- void copyConstructTask(task, composer, other)

/// \brief Copy-construct the task from the composer into \ref AnyTask
template <Task TaskType, Composer ComposerType>
constexpr inline void copyConstructTask(
    AnyTask& task,
    ComposerType& composer,
    const ComposerType& other
) {
    using ArgList = combine::detail::CopyDeps<TaskType>;
    std::apply(
        [&task, &other] (auto&... deps) {
            task.emplace<TaskType>(
                TaskCopy{}, other.template get<TaskType>(), deps...
            );
        }, composer.getList(ArgList{})
    );
} // <-- void copyConstructTask(task, composer, other)

/// \brief Move-construct the task for the composer
template <Task TaskType, Composer ComposerType>
constexpr inline void moveConstructTask(
    std::optional<TaskType>& task,
    ComposerType& composer,
    ComposerType& other
) {
    using ArgList = combine::detail::MoveDeps<TaskType>;
    std::apply(
        [&task, &other] (auto&... deps) {
            task.emplace(
                TaskMove{},
                std::move(other.template get<TaskType>()),
                deps...
            );
        }, composer.getList(ArgList{})
    );
} // <-- void moveConstructTask(task, composer, other)

/// \brief Move-construct the task for the composer into std::any
template <Task TaskType, Composer ComposerType>
constexpr inline void moveConstructTask(
    AnyTask& task,
    ComposerType& composer,
    ComposerType& other
) {
    using ArgList = combine::detail::MoveDeps<TaskType>;
    std::apply(
        [&task, &other] (auto&... deps) {
            task.emplace<TaskType>(
                TaskMove{},
                std::move(other.template get<TaskType>()),
                deps...
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

} // <-- namespace noa::utils::combine::task_manip
