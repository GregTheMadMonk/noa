/**
 * \file dynamic_composer.hh
 * \brief Definition of Combine's dynamic task composer
 */
#pragma once

#include <iostream>
#include <algorithm>
#include <functional>
#include <typeinfo>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include <noa/utils/common/meta.hh>

#include "static_composer.hh" // detail::Unroll
#include "task_manip.hh"
#include "task_traits.hh"

namespace noa::utils::combine {

/**
 * \brief Dynamic composer performs dependency resolution at runtime
 *
 * This class allows the user to specify what tasks are needed to be
 * executed at runtime, unlike with \brief StaticComposer.
 *
 * \tparam TaskList tasks for which this composer will enable dynamic
 *         dependency resoltion. All of the tasks specified here will also
 *         lead to their dependencies being recursively added to the list
 *         to avoid situations where a task's dependency is unable to be
 *         dynamically resolved due to a mistake. Dependency expantion
 *         algorithm from \ref StaticComposer is used
 */
template <Task... TaskList>
class DynamicComposer {
    /// \brief All tasks that could possibly be handled by this composer
    using Tasks = detail::Unroll< meta::List<TaskList...> >::Type;

    /**
     * \brief `std::out_of_range` child for when there was no task of type
     *        `TaskType` found
     */
    template <Task TaskType> struct NoTaskError : std::out_of_range {
        NoTaskError() : std::out_of_range{
            std::string{ "Task " }
            + taskName<TaskType>().data()
            + " not found!"
        } {}
    }; // <-- struct NoTaskError<TaskType>

    /**
     * \brief Task storage
     *
     * Tasks are stored in the order of execution in a vector
     */
    std::vector<AnyTask> tasks;

public:
    /** \brief Dynamic task initializer
     *
     * Initializers for static tasks are statically polymorphic functions.
     * We can't use this approach here, so a wrapper is required!
     */
    class Initializer {
        std::function<void(AnyTask&)> callback = nullptr;

    public:
        template <typename Func>
        Initializer(Func&& f) {
            this->callback = [f] (AnyTask& anyT) {
                const auto invokeWith =
                    [f, &anyT] <typename T> (meta::TypeTag<T>) {
                        if constexpr (std::invocable<Func, T&>) {
                            if (typeid(T) == anyT.type()) {
                                f(anyT.get<T>());
                            }
                        }
                    };

                [&invokeWith] <typename... Ts> (meta::List<Ts...>) {
                    ( invokeWith(meta::TypeTag<Ts>{}), ... );
                } (Tasks{});
            };
        } // <-- Initializer(f)

        template <std::invocable<AnyTask&> Func>
        Initializer(Func&& f) : callback(f) {}

        void operator()(AnyTask& anyT) const { this->callback(anyT); }
    }; // <-- class Initializer

    /// \brief Task name to `std::type_index` mapping
    static inline
    const std::unordered_map<std::string, std::type_index> namesMap {
        [] <Task... Ts> (meta::List<Ts...>) {
            std::unordered_map<std::string, std::type_index> ret{};

            const auto push = [&ret] <Task T> (meta::TypeTag<T>) {
                ret.emplace(taskName<T>().data(), typeid(T));
            };

            ( push(meta::TypeTag<Ts>{}), ... );

            return ret;
        } (Tasks{})
    };

private:
    /// \brief A vector of tasks' type indices
    using TaskIdVec = std::vector<std::type_index>;
    /// \brief Task dependencies
    static inline
    const std::unordered_map<std::type_index, TaskIdVec> dependencies {
        [] <typename... Ts> (meta::List<Ts...>) {
            std::unordered_map<std::type_index, TaskIdVec> ret{};

            const auto push = [&ret] <Task T> (meta::TypeTag<T>) {
                const std::type_index idx{ typeid(T) };
                ret[idx] = [] <Task... Deps> (meta::List<Deps...>) {
                    TaskIdVec vec{};

                    ( vec.emplace_back(typeid(Deps)), ... );
                    return vec;
                } (GetDeps<T>{});
            };

            ( push(meta::TypeTag<Ts>{}), ... );
            return ret;
        } (Tasks{})
    };

    /// \brief Dynamic task constructor
    using Constructor = std::function<void(AnyTask&, DynamicComposer&)>;
    /// \brief Task default constructors
    static inline
    const std::unordered_map<std::type_index, Constructor> constructTask {
        [] <Task... Ts> (meta::List<Ts...>) {
            std::unordered_map<std::type_index, Constructor> ret{};

            const auto push = [&ret] <Task T> (meta::TypeTag<T>) {
                const std::type_index idx{ typeid(T) };
                ret[idx] = [] (AnyTask& taskAny, DynamicComposer& self) {
                    task_manip::constructTask<T>(taskAny, self);
                };
            };

            ( push(meta::TypeTag<Ts>{}), ... );
            return ret;
        } (Tasks{})
    };

    /// \brief Dynamic task copier
    using Copier = std::function<
        void(AnyTask&, DynamicComposer&, const DynamicComposer&)
    >;
    /// \brief Task copiers
    static inline
    const std::unordered_map<std::type_index, Copier> copiers {
        [] <Task... Ts> (meta::List<Ts...>) {
            std::unordered_map<std::type_index, Copier> ret{};

            const auto push = [&ret] <Task T> (meta::TypeTag<T>) {
                const std::type_index idx{ typeid(T) };
                ret[idx] = [] (
                    AnyTask& task,
                    DynamicComposer& comp,
                    const DynamicComposer& other
                ) {
                    task_manip::copyConstructTask<T>(task, comp, other);
                };
            };

            ( push(meta::TypeTag<Ts>{}), ... );
            return ret;
        } (Tasks{})
    };

    /// \brief Dynamic task mover
    using Mover = std::function<
        void(AnyTask&, DynamicComposer&, DynamicComposer&&)
    >;
    /// \brief Task movers
    static inline
    const std::unordered_map<std::type_index, Mover> movers {
        [] <Task... Ts> (meta::List<Ts...>) {
            std::unordered_map<std::type_index, Mover> ret{};

            const auto push = [&ret] <Task T> (meta::TypeTag<T>) {
                const std::type_index idx{ typeid(T) };
                ret[idx] = [] (
                    AnyTask& task,
                    DynamicComposer& comp,
                    DynamicComposer&& other
                ) {
                    task_manip::moveConstructTask<T>(task, comp, other);
                };
            };

            ( push(meta::TypeTag<Ts>()), ... );
            return ret;
        } (Tasks{})
    };

    /// \brief Dynamic task update status getter
    using UpdatedGetter = std::function<bool(AnyTask&)>;
    /// \brief Task update status checkers
    static inline
    const std::unordered_map<std::type_index, UpdatedGetter> getUpdated {
        [] <Task... Ts> (meta::List<Ts...>) {
            std::unordered_map<std::type_index, UpdatedGetter> ret{};

            const auto push = [&ret] <Task T> (meta::TypeTag<T>) {
                const std::type_index idx{ typeid(T) };
                ret[idx] = [] (AnyTask& taskAny) {
                    if constexpr (UpdatableTask<T>) {
                        return taskAny.get<T>().updated();
                    } else return false;
                };
            };

            ( push(meta::TypeTag<Ts>{}), ... );
            return ret;
        } (Tasks{})
    };

    /// \brief Task updater
    using Updater = std::function<void(AnyTask&, AnyTask&)>;
    /// \brief Task updaters
    static inline
    const std::unordered_map<std::type_index, Updater> updaters {
        [] <Task... Ts> (meta::List<Ts...>) {
            std::unordered_map<std::type_index, Updater> ret{};

            const auto push = [&ret] <Task T> (meta::TypeTag<T>) {
                const std::type_index idx{ typeid(T) };

                ret[idx] = [] (AnyTask& taskAny, AnyTask& other) {
                    if (taskAny.type() == other.type()) return;

                    auto& t = taskAny.get<T>();

                    const auto tryUpdate =
                        [&t, &other] <Task Oth> (meta::TypeTag<Oth>) {
                            if constexpr (
                                requires (Oth& o) { t.onUpdated(o); }
                            ) {
                                if (typeid(Oth) == other.type()) {
                                    t.onUpdated(other.get<Oth>());
                                }
                            }
                        };

                    [&tryUpdate] <Task... Ts1> (meta::List<Ts1...>) {
                        ( tryUpdate(meta::TypeTag<Ts1>{}), ... );
                    } (Tasks{});
                };
            };

            ( push(meta::TypeTag<Ts>{}), ... );
            return ret;
        } (Tasks{})
    };

    /// \brief Task runner
    using Runner = std::function<void(AnyTask&, DynamicComposer&)>;
    /// \brief Task runners
    static inline
    const std::unordered_map<std::type_index, Runner> runners {
        [] <Task... Ts> (meta::List<Ts...>) {
            std::unordered_map<std::type_index, Runner> ret{};

            const auto push = [&ret] <Task T> (meta::TypeTag<T>) {
                const std::type_index idx{ typeid(T) };
                ret[idx] = [] (AnyTask& task, DynamicComposer& comp) {
                    task_manip::runTask(task.get<T>(), comp);
                };
            };

            ( push(meta::TypeTag<Ts>{}), ... );
            return ret;
        } (Tasks{})
    };

public:
    /// \brief Defaunt constructor does nothing
    DynamicComposer() = default;

    /// \brief Copy-constructor (calls copy-assignment)
    DynamicComposer(const DynamicComposer& other) {
        *this = other;
    } // <-- DynamicComposer(const DynamicComposer&)

    /// \brief Move-constructor (calls move-assignment)
    DynamicComposer(DynamicComposer&& other) {
        *this = std::move(other);
    } // <-- DynamicComposer(DynamicComposer&&)

    /**
     * \brief Copy-assignment
     *
     * Requires all possibly contained tasks to be copyable
     */
    DynamicComposer& operator=(const DynamicComposer& other)
    requires (allCopyable(Tasks{})) {
        const auto taskNum = other.tasks.size();
        this->tasks.resize(taskNum);
        for (std::size_t i = 0; i < taskNum; ++i) {
            const auto& otherTask = other.tasks.at(i);
            const std::type_index idx{ otherTask.type() };
            copiers.at(idx)(this->tasks[i], *this, other);
        }
        return *this;
    } // <-- DynamicComposer& operator=(const DynamicComposer&)

    /**
     * \brief Move-assignemnt
     *
     * Requires all possibly contained tasks to be movable
     */
    DynamicComposer& operator=(DynamicComposer&& other) {
        const auto taskNum = other.tasks.size();
        this->tasks.resize(taskNum);
        for (std::size_t i = 0; i < taskNum; ++i) {
            const auto& otherTask = other.tasks.at(i);
            const std::type_index idx{ otherTask.type() };
            movers.at(idx)(this->tasks[i], *this, std::move(other));
        }
        return *this;
    } // <-- DynamicComposer operator=(DynamicComposer&&)

    /// \brief Reset the composer state by clearing the task vector
    void reset() { this->tasks.clear(); }

    /// \brief Inititalizers vector
    using Inits = std::vector<Initializer>;

    /// \brief Set required tasks via template arguments
    template <Task... Tasks>
    void setTasks(const Inits& inits = {}, meta::List<Tasks...> = {}) {
        TaskIdVec request{};

        ( request.emplace_back(typeid(Tasks)), ... );

        this->setTasks(request, inits);
    } // <-- void setTasks(meta::List<Tasks...>)

    /// \brief Set required tasks via names
    void setTasks(
        const std::vector<std::string>& names, const Inits& inits = {}
    ) {
        TaskIdVec request{};
        for (const auto& name : names) {
            request.push_back(namesMap.at(name));
        }
        this->setTasks(request, inits);
    } // <-- void setTasks(const std::vector<std::string>& names)

private:
    /**
     * \brief Set required tasks via type indices
     *
     * All public `setTasks` function calls lead to this private overload.
     * This function processes the input vector and constructs \ref tasks
     * accordingly.
     * 
     * Previous composer state gets reset
     */
    void setTasks(const TaskIdVec& typeIdxs, const Inits& inits) {
        this->reset();

        std::vector<TaskIdVec> queue = { typeIdxs };
        while (queue.back().size() != 0) {
            TaskIdVec next{};
            for (const auto taskIdx : queue.back()) {
                for (const auto idx : dependencies.at(taskIdx)) {
                    next.push_back(idx);
                }
            }

            queue.push_back(std::move(next));
        }

        // Go from the back of the queue adding tasks
        for (auto it = queue.crbegin(); it != queue.crend(); ++it) {
            for (const auto idx : *it) {
                const auto found = std::find_if(
                    this->tasks.cbegin(), this->tasks.cend(),
                    [idx] (const auto& anyT) {
                        return anyT.type() == idx;
                    }
                );
                // Only add unique tasks
                if (found != this->tasks.cend()) continue;

                this->tasks.emplace_back();
                constructTask.at(idx)(this->tasks.back(), *this);

                for (const auto& init : inits) init(this->tasks.back());
            }
        }
    } // <-- void setTasks(typeIdxs)

public:
    /// \brief Run the tasks
    void run() {
        for (auto t = this->tasks.begin(); t != this->tasks.end(); ++t) {
            if (getUpdated.at(t->type())(*t)) {
                for (auto o = std::next(t); o != this->tasks.end(); ++o) {
                    updaters.at(o->type())(*o, *t);
                }
            }

            runners.at(t->type())(*t, *this);
        }
    } // <-- void run()

    // Compatibility functions for 'static' task getters
    /// \brief Get task reference by type
    template <Task TaskType>
    TaskType& get() {
        const auto it = std::find_if(
            this->tasks.begin(), this->tasks.end(),
            [] (const auto& anyT) {
                return anyT.type() == typeid(TaskType);
            }
        );

        if (it == this->tasks.end()) throw NoTaskError<TaskType>{};

        return it->template get<TaskType>();
    } // <-- TaskType& get()
    /// \brief Get task const reference by type
    template <Task TaskType>
    const TaskType& get() const {
        const auto it = std::find_if(
            this->tasks.cbegin(), this->tasks.cend(),
            [] (const auto& anyT) {
                return anyT.type() == typeid(TaskType);
            }
        );

        if (it == this->tasks.cend()) throw NoTaskError<TaskType>{};

        return it->template get<TaskType>();
    } // <-- TaskType& get()
    /// \brief Get several tasks as a tuple of references
    template <Task... TasksList>
    auto getList(meta::List<TasksList...> = {}) {
        return std::tie(this->get<TasksList>()...);
    } // <-- auto getList(meta::List<TasksList...>)
    /// \brief Get several tasks as a tuple of const references
    template <Task... TasksList>
    auto getList(meta::List<TasksList...> = {}) const {
        return std::tie(this->get<TasksList>()...);
    } // <-- auto getList(meta::List<TasksList...>) const
}; // <-- class DynamicComposer<TaskList...>

} // <-- namespace noa::utils::combine
