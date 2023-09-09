/**
 * \file any_task.hh
 * \brief Simplified `std::any`-like container for tasks
 */
#pragma once

#include <concepts>
#include <functional>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <typeindex>

#include "task_traits.hh"

namespace noa::utils::combine {

/**
 * \brief Thrown when trying to access \ref AnyTask by type that is not
 *        stored in it
 */
template <Task TaskType>
struct BadAnyTaskAccess : std::runtime_error {
    BadAnyTaskAccess() : std::runtime_error{
        std::string{"Task "} + taskName<TaskType>().data()
        + " is not stored in this container"
    } {}
};

/**
 * \brief Simplified `std::any`-like container for tasks
 *
 * Does not require the type to be copy-constructible. Is not copyable
 * OR movable.
 */
class AnyTask {
    /// \brief The stored object pointer. Yes, I know
    void* data = nullptr;
    /// \brief The stored object type info
    std::type_index typeIndex{ typeid(void) };
    /// \brief Stored object destructor
    std::function<void(void*)> destroy = nullptr;

public:
    /// \brief Default constructor
    AnyTask() = default;

    AnyTask(const AnyTask&) = delete;
    AnyTask& operator=(const AnyTask&) = delete;

    /// \brief Move-constructor. Calls move-assignment
    AnyTask(AnyTask&& other) { *this = std::move(other); }
    /// \brief Move-assignment
    AnyTask& operator=(AnyTask&& other) {
        this->data = other.data;
        other.data = nullptr;
        this->typeIndex = other.typeIndex;
        other.typeIndex = typeid(void);
        this->destroy = other.destroy;
        other.destroy = nullptr;
        return *this;
    } // <-- AnyTask& operator=(AnyTask&&)

    /// \brief Construct a task inside of this container
    template <Task TaskType, typename... Args>
    requires std::constructible_from<TaskType, Args&&...>
    void emplace(Args&&... args) {
        this->data = new TaskType(std::forward<Args>(args)...);
        this->typeIndex = typeid(TaskType);
        this->destroy = [] (void* ptr) {
            auto* taskPtr = reinterpret_cast<TaskType*>(ptr);
            delete taskPtr;
        };
    }

    /// \brief Get the stored value as reference
    template <Task TaskType> TaskType& get()
    {
        if (this->typeIndex != typeid(TaskType)) {
            throw BadAnyTaskAccess<TaskType>{};
        }
        return *reinterpret_cast<TaskType*>(this->data);
    }
    /// \brief Get the stored value as const reference
    template <Task TaskType> const TaskType& get() const
    {
        if (this->typeIndex != typeid(TaskType)) {
            throw BadAnyTaskAccess<TaskType>{};
        }
        return *reinterpret_cast<const TaskType*>(this->data);
    }

    /// \brief Erase the value from the container
    void reset() {
        this->destroy(this->data);
        this->data = nullptr;
        this->typeIndex = typeid(void);
        this->destroy = nullptr;
    } // <-- void reset()

    /**
     * \brief Get type index of the stored object
     *
     * \return `std::type_index` of stored object or `std::type_index` of
     *         `void` in case there is none
     */
    std::type_index type() const { return this->typeIndex; }

    /// \brief The destructor
    ~AnyTask() {
        if (this->data != nullptr) this->reset();
    } // <-- ~AnyTask()
}; // <-- class AnyTask

} // <-- namespace noa::utils::combine
