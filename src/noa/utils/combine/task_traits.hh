/**
 * \file task_traits.hh
 * \brief Type traits for Combine tasks
 */
#pragma once

#include <string_view>
#include <typeinfo>

#include <noa/utils/common/meta.hh>

namespace noa::utils::combine {

/// \brief Task copy-constructor indicator
struct TaskCopy {};

/// \brief Task move-constructor indicator
struct TaskMove {};

/// \brief Checks if a type specifies a task
///
/// Returns `true` if:
template <typename TaskCandidate>
concept Task = requires {
    /// * it has a constructor that is not a move- or copy- constructor
    requires meta::InstanceOf<
        meta::GetConstructorArgs<TaskCandidate>,
        meta::List
    >;
    requires !meta::GetConstructorArgs<TaskCandidate>
                                        ::template contains<TaskCopy>;
    requires !meta::GetConstructorArgs<TaskCandidate>
                                        ::template contains<TaskMove>;
    /// * has a `run` method
    requires meta::InstanceOf<
        meta::GetCallableArgs<&TaskCandidate::run>,
        meta::List
    >;
}; // <-- concept Task

/// \brief Checks if a task is copyable (has a constructor with
///        \ref TaskCopy first argumennt)
template <typename TaskType>
concept CopyableTask = requires {
    requires Task<TaskType>;
    requires meta::InstanceOf<
        meta::GetConstructorArgs<TaskType, TaskCopy>,
        meta::List
    >;
    requires std::same_as<
        std::remove_cv_t<
            meta::At<meta::GetConstructorArgs<TaskType, TaskCopy>, 1>
        >, TaskType
    >;
}; // <-- concept CopyableTask
/// \brief Checks if a task is movable (has a constructor with
///        \ref TaskMove first argument)

template <typename TaskType>
concept MovableTask = requires {
    requires Task<TaskType>;
    requires meta::InstanceOf<
        meta::GetConstructorArgs<TaskType, TaskMove>,
        meta::List
    >;
    requires std::same_as<
        std::remove_cv_t<
            meta::At<meta::GetConstructorArgs<TaskType, TaskMove>, 1>
        >, TaskType
    >;
}; // <-- concept MovableTask

/// \brief Checks if a task has a name (static `name` member)
template <typename TaskType>
concept NamedTask = requires {
    requires Task<TaskType>;
    requires
        std::convertible_to<decltype(TaskType::name), std::string_view>;
}; // <-- concept NamedTask

/// \brief Get task name
template <Task TaskType>
std::string_view taskName() {
    if constexpr (NamedTask<TaskType>) {
        return TaskType::name;
    } else {
        return typeid(TaskType).name();
    }
} // <-- std::string_view taskName()

/**
 * \brief Is the task updatable?
 *
 * Some tasks may have a public updated() function
 */
template <typename TaskType>
concept UpdatableTask = requires (const TaskType& ctask) {
    requires Task<TaskType>;
    { ctask.updated() } -> std::same_as<bool>;
};

namespace detail {

/// \brief Get task constructor dependencies
template <Task TaskType>
using ConstructorDeps = meta::GetConstructorArgs<TaskType>
                        ::template Apply<std::remove_cvref_t>::Unique;
/// \brief Get task `run` method dependencies
template <Task TaskType>
using RunDeps = meta::GetCallableArgs<&TaskType::run>
                        ::template Apply<std::remove_cvref_t>::Unique;
/// \brief Get task copy constructor dependencies
template <Task TaskType>
using CopyDeps = std::conditional_t<
    CopyableTask<TaskType>,
    typename meta::GetConstructorArgs<TaskType, TaskCopy>,
    meta::List<void, void>
>::template Crop<2>::template Apply<std::remove_cvref_t>::Unique;
/// \brief Get task move constructor dependencies
template <Task TaskType>
using MoveDeps = std::conditional_t<
    MovableTask<TaskType>,
    typename meta::GetConstructorArgs<TaskType, TaskMove>,
    meta::List<void, void>
>::template Crop<2>::template Apply<std::remove_cvref_t>::Unique;

} // <-- namespace detail

/// \brief Get task dependencies
template <Task TaskType>
using GetDeps = meta::Concat<
    detail::ConstructorDeps<TaskType>,
    detail::RunDeps<TaskType>,
    detail::CopyDeps<TaskType>,
    detail::MoveDeps<TaskType>
>::Unique::template Apply<std::remove_cvref_t>; // <-- using GetDeps

// Task lists processing

/// \brief Are all tasks in the list copyable?
template <Task... Tasks>
consteval bool allCopyable(meta::List<Tasks...> = {}) {
    return (CopyableTask<Tasks> && ...);
}

/// \brief Are all tasks in the list movable?
template <Task... Tasks>
consteval bool allMovable(meta::List<Tasks...> = {}) {
    return (MovableTask<Tasks> && ...);
}


#ifdef NOA_COMPILE_TIME_TESTS
namespace test::task_traits {

struct NotATask {};
struct NotATask2 { NotATask2() = delete; void run(); };
struct IsTask1 { void run(); };
struct IsTask2 { void run(const IsTask1&); };
struct IsCopyableTask1 {
    IsCopyableTask1();
    IsCopyableTask1(TaskCopy, const IsCopyableTask1&);
    void run();
};
struct IsCopyableTask2 {
    IsCopyableTask2(IsTask2);
    IsCopyableTask2(TaskCopy, const IsCopyableTask2&);
    void run(IsTask1, IsTask2);
};
struct IsMovableTask1 {
    IsMovableTask1();
    IsMovableTask1(TaskMove, IsMovableTask1&&);
    void run();
};
struct IsMovableTask2 {
    IsMovableTask2(IsTask2);
    IsMovableTask2(TaskMove, IsMovableTask2&&);
    void run(IsTask1, IsTask2);
};
struct IsCopyableMovableTask {
    IsCopyableMovableTask(IsTask2);
    IsCopyableMovableTask(TaskCopy, const IsCopyableMovableTask&);
    IsCopyableMovableTask(TaskMove, IsCopyableMovableTask&&);

    void run();
};

static_assert(!Task<NotATask>);
static_assert(!Task<NotATask2>);
static_assert(Task<IsTask1>);
static_assert(Task<IsTask2>);
static_assert(!MovableTask<IsTask1>);
static_assert(!MovableTask<IsTask2>);
static_assert(!CopyableTask<IsTask1>);
static_assert(!CopyableTask<IsTask2>);
static_assert(Task<IsCopyableTask1>);
static_assert(Task<IsCopyableTask2>);
static_assert(!MovableTask<IsCopyableTask1>);
static_assert(!MovableTask<IsCopyableTask2>);
static_assert(CopyableTask<IsCopyableTask1>);
static_assert(CopyableTask<IsCopyableTask2>);
static_assert(Task<IsMovableTask1>);
static_assert(Task<IsMovableTask2>);
static_assert(MovableTask<IsMovableTask1>);
static_assert(MovableTask<IsMovableTask2>);
static_assert(!CopyableTask<IsMovableTask1>);
static_assert(!CopyableTask<IsMovableTask2>);
static_assert(Task<IsCopyableMovableTask>);
static_assert(MovableTask<IsCopyableMovableTask>);
static_assert(CopyableTask<IsCopyableMovableTask>);

static_assert(std::same_as< GetDeps<IsTask1>,  meta::List<> >);

static_assert(std::same_as< GetDeps<IsTask2>, meta::List<IsTask1> >);

static_assert(std::same_as< GetDeps<IsCopyableTask1>, meta::List<> >);
static_assert(
    std::same_as<
        GetDeps<IsCopyableTask2>, meta::List<IsTask2, IsTask1>
    >
);

static_assert(std::same_as< GetDeps<IsMovableTask1>, meta::List<> >);
static_assert(
    std::same_as<
        GetDeps<IsMovableTask2>, meta::List<IsTask2, IsTask1>
    >
);

static_assert(
    std::same_as< GetDeps<IsCopyableMovableTask>, meta::List<IsTask2> >
);

struct IsUpdatableTask1 { void run(); bool updated() const; };

static_assert(!UpdatableTask<IsTask1>);
static_assert(UpdatableTask<IsUpdatableTask1>);

} // <-- namespace test::task_traits
#endif

} // <-- namespace noa::utils::combine
