/**
 * \file static_composer.hh
 * \brief Definition of Combine's static task composer.
 */

#pragma once

#include <optional>
#include <tuple>

#include "task_manip.hh"
#include "task_traits.hh"

namespace noa::combine {

namespace detail {

    template <
        typename LastList,
        utils::meta::InstanceOf<utils::meta::List>... Lists
    > struct Unroll {
        using Type = Unroll<
            typename utils::meta::Cast<
                typename LastList::template Apply<GetDeps>, utils::meta::Concat
            >,
            typename LastList::Unique, Lists...
        >::Type;
    };

    template < utils::meta::InstanceOf<utils::meta::List>... Lists >
    struct Unroll<void, Lists...> {
        using Type = utils::meta::Concat< Lists... >::Unique;
    };

    template <typename Initializer, utils::meta::InstanceOf<utils::meta::List> Tasks>
    struct ValidateInitializer;

    template <typename Initializer, Task... Tasks>
    struct ValidateInitializer<Initializer, utils::meta::List<Tasks...>> {
        static constexpr auto value =
            ( std::invocable<Initializer, Tasks&> || ... || false );
    };

    template <typename Initializer, typename Tasks>
    concept ValidInitializer =
        ValidateInitializer<Initializer, Tasks>::value;

} // <-- detail

/**
 * \brief Performs task dependency resolution and composition at compile-time
 *
 * _Note: "static" only refers to task composition and dependency resolution.
 * The actual computations are still performed at runtime_.
 *
 * Required tasks are specified as veriadic template arguments and cannot be
 * changed at runtime.
 */
template <Task... Tasks>
class StaticComposer {
    /// Full task sequence (first to last, including dependencies)
    using TaskSeq = detail::Unroll< utils::meta::List<Tasks...> >::Type;

    /**
     * \brief Task storage
     *
     * Optional is used to allow to define task initialization order
     */
    using TaskTup = utils::meta::Cast<
        typename TaskSeq::template Apply<std::optional>,
        std::tuple
    >;
    TaskTup tasks{};

public:
    /**
     * \brief Constructs the static composer and initializes the tasks
     *
     * \param ...initializers - functors each accepting one of the tasks
     *                          initialized by this composer (declared
     *                          explicitly or added as a dependency).
     *                          Initializers for each task are run in
     *                          oreder immediately after the task
     *                          construction but before its dependants
     *                          are countructed.
     */
    template <detail::ValidInitializer<TaskSeq>... Initializers>
    constexpr StaticComposer(Initializers&&... initializers) {
        std::apply(
            [this, &initializers...] <Task... Ts>
            (std::optional<Ts>&... taskOpts) {
                (
                    task_manip::constructTask<Ts>(
                        taskOpts, *this, initializers...
                    ), ...
                );
            }, this->tasks
        );
    } // <-- StaticComposer(initializers...)

    /// \brief Copy-constructor (calls copy-assignment)
    constexpr StaticComposer(const StaticComposer& other) {
        *this = other;
    }

    /// \brief Move-constructor (calls move-assignment)
    constexpr StaticComposer(StaticComposer&& other) {
        *this = std::move(other);
    }

    /**
     * \brief Copy-assignment
     *
     * Requires all tasks in \ref TaskSeq to be copyable
     */
    constexpr StaticComposer& operator=(const StaticComposer& other)
    requires ( allCopyable(TaskSeq{}) ) {
        std::apply(
            [this, &other] <Task... Ts> (std::optional<Ts>&... taskOpts) {
                (
                    task_manip::copyConstructTask<Ts>(
                        taskOpts, *this, other
                    ), ...
                );
            }, this->tasks
        );

        return *this;
    } // <-- StaticComposer(const StaticComposer&)

    /**
     * \brief Move-assignment
     *
     * Requires all tasks in \ref TaskSeq to be movable
     */
    constexpr StaticComposer& operator=(StaticComposer&& other)
    requires ( allMovable(TaskSeq{}) ) {
        std::apply(
            [this, &other] <Task... Ts> (std::optional<Ts>&... taskOpts) {
                (
                    task_manip::moveConstructTask<Ts>(
                        taskOpts, *this, other
                    ), ...
                );
            }, this->tasks
        );

        return *this;
    }

    /// \brief Get task reference via type
    template <Task TaskType> TaskType& get()
    { return *std::get<std::optional<TaskType>>(this->tasks); }
    /// \brief Get task const reference via type
    template <Task TaskType> const TaskType& get() const
    { return *std::get<std::optional<TaskType>>(this->tasks); }

    /// \brief Get several tasks as a tuple of references
    template <Task... TaskList>
    auto getList(utils::meta::List<TaskList...> = {}) {
        return std::tie(this->get<TaskList>()...);
    }

    /// \brief Get several tasks as a tuple of const references
    template <Task... TaskList>
    auto getList(utils::meta::List<TaskList...> = {}) const {
        return std::tie(this->get<TaskList>()...);
    }

    /// \brief Run the tasks
    void run() {
        std::apply(
            [this] (auto&... ts) {
                const auto updateTask = [] <typename T> (T& task) {
                    return [&task] (auto&... ts1) {
                        if constexpr (UpdatableTask<T>) {
                            if (!task.updated()) return;


                            (
                                [&task] <typename OtherT> (OtherT& ot) {
                                    if constexpr (
                                        !std::same_as<T, OtherT>
                                        && requires {
                                            ot.onUpdated(task);
                                        }
                                    ) {
                                        ot.onUpdated(task);
                                    }
                                } (*ts1), ...
                            );
                        }
                    };
                };
                // Check for updates
                ( updateTask(*ts)(ts...), ... );

                // Run `.run()` methods on tasks
                ( task_manip::runTask(*ts, *this), ... );
            }, this->tasks
        );
    } // <-- void run()
}; // <-- struct StaticComposer

#ifdef NOA_COMPILE_TIME_TESTS
namespace test::static_composer {

struct Task1 { void run(); };
struct Task2 {
    void run(const Task1&) {}
};
struct Task3 {
    Task3(Task1&) {}
    void run(const Task2&) {}
};
struct Task4 {
    Task4() {}
    Task4(TaskCopy, const Task4&, Task1) {}
    void run() {}
};
struct Task5 {
    Task5() {}
    Task5(TaskCopy, const Task5&) {}
    Task5(TaskMove, Task5&&, const Task4&) {}

    void run(const Task2&) {}
};

static_assert(
    std::same_as<
        detail::Unroll< utils::meta::List<Task1> >::Type,
        utils::meta::List<Task1>
    >
);

static_assert(
    std::same_as<
        detail::Unroll< utils::meta::List<Task2> >::Type,
        utils::meta::List<Task1, Task2>
    >
);

static_assert(
    std::same_as<
        detail::Unroll< utils::meta::List<Task2, Task1> >::Type,
        utils::meta::List<Task1, Task2>
    >
);

static_assert(
    std::same_as<
        detail::Unroll< utils::meta::List<Task1, Task2> >::Type,
        utils::meta::List<Task1, Task2>
    >
);

static_assert(
    std::same_as<
        detail::Unroll< utils::meta::List<Task3> >::Type,
        utils::meta::List<Task1, Task2, Task3>
    >
);

static_assert(
    std::same_as<
        detail::Unroll< utils::meta::List<Task4> >::Type,
        utils::meta::List<Task1, Task4>
    >
);

static_assert(
    std::same_as<
        detail::Unroll< utils::meta::List<Task5> >::Type,
        utils::meta::List<Task1, Task2, Task4, Task5>
    >
);

} // <-- namespace test::static_composer
#endif // NOA_COMPILE_TIME_TESTS

} // <-- namespace noa::combine
