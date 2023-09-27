/**
 * @file pywrapper.hh
 * @brief Defines wrappers used with PyBind to expose NOA classes
 */

#pragma once

namespace noa::bindings::python {

/**
 * @brief Non-owning pointer wrapper. Required to bypass auto pointer type
 *        recognition by PyBind
 */
template <typename T>
class WeakWrapper {
    T* p = nullptr;

public:
    WeakWrapper(T& obj) : p(&obj) {}
    T* operator->() { return this->p; }
    const T* operator->() const { return this->p; }
    T& get() { return *this->p; }
    const T& get() const { return *this->p; }

    WeakWrapper<const T> toConst() const
    { return WeakWrapper<const T>(*this->p); }
}; // <-- class WeakWrapper

/**
 * @brief Same as @ref WeakWrapper, but doesn't allow to modify the
 *        wrapped object
 */
template <typename T> using ConstWeakWrapper = WeakWrapper<const T>;

} // <-- namespace noa::binding::python
