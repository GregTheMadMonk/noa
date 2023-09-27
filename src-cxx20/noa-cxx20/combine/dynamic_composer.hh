/**
 * @file dynamic_composer.hh
 * @brief Definition of Combine's dynamic task composer
 */
#pragma once

#include <algorithm>
#include <functional>
#include <typeinfo>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include <noa-cxx20/utils/meta.hh>

#include "static_composer.hh" // detail::Unroll
#include "task_manip.hh"
#include "task_traits.hh"

namespace noa::utils::combine {

/**
 * @brief Dynamic composer performs dependency resolution at runtime
 *
 * This class allows the user to specify what tasks are needed to be
 * executed at runtime, unlike with @ref StaticComposer.
 *
 * @tparam TaskList tasks for which this composer will enable dynamic
 *         dependency resoltion. All of the tasks specified here will also
 *         lead to their dependencies being recursively added to the list
 *         to avoid situations where a task's dependency is unable to be
 *         dynamically resolved due to a mistake. Dependency expantion
 *         algorithm from @ref StaticComposer is used
 */
template <Task... TaskList>
class DynamicComposer {
    /// @brief All tasks that could possibly be handled by this composer
    using Tasks = detail::Unroll< meta::List<TaskList...> >::Type;

    /**
     * @brief `std::out_of_range` child for when there was no task of type
     *        `TaskType` found
     */
    template <Task TaskType> struct NoTaskError : std::out_of_range {
        NoTaskError() : std::out_of_range{
            std::string{ "Task " }
            + taskName<TaskType>().data()
            + " not found!"
        } {}
    }; // <-- struct NoTaskError<TaskType>

    /// @brief Task type index vector
    using TaskIdxVec = std::vector<std::type_index>;

    template <Task> class AsDynamic;
    /// @brief Base abstract class for dynamic task container
    struct DynamicTask {
        /// @brief Convert to specific child type reference
        template <Task TaskType>
        AsDynamic<TaskType>& as()
        { return *reinterpret_cast<AsDynamic<TaskType>*>(this); }
        /// @brief Convert to specific child type const reference
        template <Task TaskType>
        const AsDynamic<TaskType>& as() const
        { return *reinterpret_cast<const AsDynamic<TaskType>*>(this); }

        /// @brief Stored task type index
        virtual std::type_index type() const = 0;
        /// @brief Run the stored task
        virtual void run(DynamicComposer&) = 0;
        /// @brief Check if the task was updated
        virtual bool updated() const = 0;
        /**
         * @brief Call the `onUpdated()` function on th stored task with
         *        another task if available
         */
        virtual void onUpdated(DynamicTask&) = 0;
        /// @brief Get task dependencies
        virtual const TaskIdxVec& dependencies() const = 0;

        virtual ~DynamicTask() {}

        template <Task TaskType, typename... Args>
        friend
        void emplace(std::unique_ptr<DynamicTask>& ptr, Args&&... args) {
            ptr = std::move(
                std::unique_ptr<DynamicTask>(
                    new AsDynamic<TaskType>(std::forward<Args>(args)...)
                )
            );
        } // <-- void emplace(ptr, args...)
    }; // <-- class DynamicTask

    /// @brief Dynamic wrapper for a task type
    template <Task TaskType>
    struct AsDynamic : public DynamicTask {
        TaskType task;

        template <typename... Args>
        AsDynamic(Args&&... args) : task(std::forward<Args>(args)...) {}

        std::type_index type() const override { return typeid(TaskType); }

        void run(DynamicComposer& comp) override {
            task_manip::runTask(this->task, comp);
        } // <-- void run(comp)

        bool updated() const {
            if constexpr (UpdatableTask<TaskType>) {
                return this->task.updated();
            } else return false;
        } // <-- bool updated()

        void onUpdated(DynamicTask& other) override {
            // Can't update by itself
            if (typeid(TaskType) == other.type()) return;

            const auto updater =
                [&t=this->task, &other] <Task O> (meta::TypeTag<O>) {
                    if constexpr (requires (O& o) { t.onUpdated(o); }) {
                        if (typeid(O) != other.type()) return;

                        t.onUpdated(other.template as<O>().task);
                    }
                };

            [&updater] <Task... AllTasks> (meta::List<AllTasks...>) {
                ( updater(meta::TypeTag<AllTasks>{}), ... );
            } (Tasks{});
        } // <-- void onUpdated(other)

        static inline const TaskIdxVec deps =
            [] <Task... Deps> (meta::List<Deps...>) {
                TaskIdxVec ret{};
                
                ( ret.push_back(typeid(Deps)), ... );
                return ret;
            } (GetDeps<TaskType>{});

        const TaskIdxVec& dependencies() const override {
            return deps;
        } // <-- TaskIdxVec dependencies() const
    }; // <-- class AsDynamic

    /**
     * @brief Task storage
     *
     * Tasks are stored in the order of execution in a vector
     */
    std::vector<std::unique_ptr<DynamicTask>> tasks;

    /**
     * @brief Convert runtime task type info to compile-time type
     *
     * Calls `f` with `meta::TypeTag<T>` argument (like std::visit), where
     * `typeid(T) == idx`
     */
    template <typename Func>
    static inline void visit(Func&& f, std::type_index idx) {
        [idx, &f] <Task... AllTasks> (meta::List<AllTasks...>) {
            (
                [idx, &f] <Task Task> (meta::TypeTag<Task>) {
                    using Arg = meta::TypeTag<Task>;
                    if constexpr (std::invocable<Func, Arg>) {
                        if (typeid(Task) == idx) f(Arg{});
                    }
                } (meta::TypeTag<AllTasks>{}), ...
            );
        } (Tasks{});
    } // <-- static auto visit(idx, f)
    /**
     * @brief Converts runtime task type info to compile-time to call the
     *        functor with a proper type
     */
    template <typename Func>
    static inline void visit(Func&& f, DynamicTask& task) {
        visit(
            [&task, &f] <Task TaskType> (meta::TypeTag<TaskType>) {
                f(task.template as<TaskType>().task);
            }, task.type()
        );
    } // <-- static void visit(task, f)
    /**
     * @brief Converts runtime task type info to compile-time to call the
     *        functor with a proper type if possible
     */
    template <typename Func>
    static inline void tryVisit(Func&& f, DynamicTask& task) {
        visit(
            [&task, &f] <Task TaskType> (meta::TypeTag<TaskType>) {
                if constexpr (std::invocable<Func, TaskType&>) {
                    f(task.template as<TaskType>().task);
                }
            }, task.type()
        );
    } // <-- static void visit(task, f)

public:
    /** @brief Dynamic task initializer
     *
     * Initializers for static tasks are statically polymorphic functions.
     * We can't use this approach here, so a wrapper is required!
     */
    class Initializer {
        std::function<void(DynamicTask&)> callback = nullptr;

    public:
        /// @brief Construct from an arbitrary functor
        template <typename Func>
        Initializer(Func&& f) {
            this->callback =
                [f] (DynamicTask& task) { tryVisit(f, task); };
        } // <-- Initializer(f)

        /// @brief Construct from a functor invokable with `DynamicTask&`
        template <std::invocable<DynamicTask&> Func>
        Initializer(Func&& f) : callback(f) {}

        void operator()(DynamicTask& task) const { this->callback(task); }
    }; // <-- class Initializer

    /// @brief Task name to `std::type_index` mapping
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

    /// @brief Defaunt constructor does nothing
    DynamicComposer() = default;

    /// @brief Copy-constructor (calls copy-assignment)
    DynamicComposer(const DynamicComposer& other) {
        *this = other;
    } // <-- DynamicComposer(const DynamicComposer&)

    /// @brief Move-constructor (calls move-assignment)
    DynamicComposer(DynamicComposer&& other) {
        *this = std::move(other);
    } // <-- DynamicComposer(DynamicComposer&&)

    /**
     * @brief Copy-assignment all possibly contained tasks to be copyable
     */
    DynamicComposer& operator=(const DynamicComposer& other)
    requires (allCopyable(Tasks{})) {
        const auto taskNum = other.tasks.size();
        this->tasks.resize(taskNum);
        for (std::size_t i = 0; i < taskNum; ++i) {
            const auto& otherTask = other.tasks.at(i);
            visit(
                [this, i, &other] <Task T> (meta::TypeTag<T>) {
                    task_manip::copyConstructTask<T>(
                        this->tasks[i], *this, other
                    );
                }, otherTask->type()
            );
        }
        return *this;
    } // <-- DynamicComposer& operator=(const DynamicComposer&)

    /**
     * @brief Move-assignemnt
     *
     * Requires all possibly contained tasks to be movable
     */
    DynamicComposer& operator=(DynamicComposer&& other) {
        const auto taskNum = other.tasks.size();
        this->tasks.resize(taskNum);
        for (std::size_t i = 0; i < taskNum; ++i) {
            const auto& otherTask = other.tasks.at(i);
            visit(
                [this, i, &other] <Task T> (meta::TypeTag<T>) {
                    task_manip::moveConstructTask<T>(
                        this->tasks[i], *this, other
                    );
                }, otherTask->type()
            );
        }
        return *this;
    } // <-- DynamicComposer operator=(DynamicComposer&&)

    /// @brief Reset the composer state by clearing the task vector
    void reset() { this->tasks.clear(); }

    /// @brief Inititalizers vector
    using Inits = std::vector<Initializer>;

    /// @brief Set required tasks via template arguments
    template <Task... Tasks>
    void setTasks(const Inits& inits = {}, meta::List<Tasks...> = {}) {
        TaskIdxVec request{};

        ( request.emplace_back(typeid(Tasks)), ... );

        this->setTasks(request, inits);
    } // <-- void setTasks(meta::List<Tasks...>)

    /// @brief Set required tasks via names
    void setTasks(
        const std::vector<std::string>& names, const Inits& inits = {}
    ) {
        TaskIdxVec request{};
        for (const auto& name : names) {
            request.push_back(namesMap.at(name));
        }
        this->setTasks(request, inits);
    } // <-- void setTasks(const std::vector<std::string>& names)

private:
    /**
     * @brief Set required tasks via type indices
     *
     * All public `setTasks` function calls lead to this private overload.
     * This function processes the input vector and constructs @ref tasks
     * accordingly.
     * 
     * Previous composer state gets reset
     */
    void setTasks(const TaskIdxVec& typeIdxs, const Inits& inits) {
        this->reset();

        std::vector<TaskIdxVec> queue = { typeIdxs };
        while (queue.back().size() != 0) {
            TaskIdxVec next{};
            for (const auto taskIdx : queue.back()) {
                visit(
                    [&next] <Task DepTask> (meta::TypeTag<DepTask>) {
                        for (const auto idx : AsDynamic<DepTask>::deps) {
                            next.push_back(idx);
                        }
                    }, taskIdx
                );
            }

            queue.push_back(std::move(next));
        }

        // Go from the back of the queue adding tasks
        for (auto it = queue.crbegin(); it != queue.crend(); ++it) {
            for (const auto idx : *it) {
                const auto found = std::find_if(
                    this->tasks.cbegin(), this->tasks.cend(),
                    [idx] (const auto& anyT) {
                        return anyT->type() == idx;
                    }
                );
                // Only add unique tasks
                if (found != this->tasks.cend()) continue;

                this->tasks.emplace_back();
                visit(
                    [this] <Task T> (meta::TypeTag<T>) {
                        task_manip::constructTask<T>(
                            this->tasks.back(), *this
                        );
                    }, idx
                );

                for (const auto& init : inits) init(*this->tasks.back());
            }
        }
    } // <-- void setTasks(typeIdxs)

public:
    /// @brief Get all the possible task names
    static inline std::vector<std::string> getAllowedTasks() {
        return [] <Task... AllTasks> (meta::List<AllTasks...>) {
            std::vector<std::string> ret{};

            (
                [&ret] <Task TaskType> (meta::TypeTag<TaskType>) {
                    if constexpr (NamedTask<TaskType>) {
                        ret.push_back(taskName<TaskType>().data());
                    }
                } (meta::TypeTag<AllTasks>{}), ...
            );

            return ret;
        } (Tasks{});
    } // <-- static inline std::vector<std::string> getAllowedTasks()

    /// @brief Run the tasks
    void run() {
        for (auto t = this->tasks.begin(); t != this->tasks.end(); ++t) {
            if ((*t)->updated()) {
                for (auto o = std::next(t); o != this->tasks.end(); ++o) {
                    (*o)->onUpdated(*(*t));
                }
            }

            (*t)->run(*this);
        }
    } // <-- void run()

    // Compatibility functions for 'static' task getters
    /// @brief Get task reference by type
    template <Task TaskType>
    TaskType& get() {
        const auto it = std::find_if(
            this->tasks.begin(), this->tasks.end(),
            [] (const auto& anyT) {
                return anyT->type() == typeid(TaskType);
            }
        );

        if (it == this->tasks.end()) throw NoTaskError<TaskType>{};

        return (*it)->template as<TaskType>().task;
    } // <-- TaskType& get()
    /// @brief Get task const reference by type
    template <Task TaskType>
    const TaskType& get() const {
        const auto it = std::find_if(
            this->tasks.cbegin(), this->tasks.cend(),
            [] (const auto& anyT) {
                return anyT->type() == typeid(TaskType);
            }
        );

        if (it == this->tasks.cend()) throw NoTaskError<TaskType>{};

        return (*it)->template as<TaskType>().task;
    } // <-- TaskType& get()
    /// @brief Get several tasks as a tuple of references
    template <Task... TasksList>
    auto getList(meta::List<TasksList...> = {}) {
        return std::tie(this->get<TasksList>()...);
    } // <-- auto getList(meta::List<TasksList...>)
    /// @brief Get several tasks as a tuple of const references
    template <Task... TasksList>
    auto getList(meta::List<TasksList...> = {}) const {
        return std::tie(this->get<TasksList>()...);
    } // <-- auto getList(meta::List<TasksList...>) const
}; // <-- class DynamicComposer<TaskList...>

} // <-- namespace noa::utils::combine
