/**
 * @file tnlx.hh
 * @brief Extensions to the TNL library
 */
#pragma once

// Standard library
#include <concepts>

// TNL headers
#include <noa/3rdparty/tnl-noa/src/TNL/Containers/Array.h>

// NOA headers
#include <noa/utils/common/meta.hh>

/// @brief Extensions to the TNL library
namespace noa::utils::tnl {

template <typename ArrayLike> using Index = ArrayLike::IndexType;

namespace detail {

    template <typename ArrayLike> struct Value {
        using Type = ArrayLike::RealType;
    }; // <-- struct Value<ArrayLike>

    template <typename ArrayLike>
    struct Value<const ArrayLike> {
        using Type = const ArrayLike::RealType;
    }; // <-- struct Value<const ArrayLike>

} // <-- namespace detail

template <typename ArrayLike>
using Value = detail::Value<ArrayLike>::Type;

/**
 * @brief Concept specifying a TNL array or an array view. `T` is allowed
 *        to provide only `const` access to its elements
 */
template <typename T>
concept LinearContainer = requires (T t) {
    // Has a valid `size()` member function
    { t.getSize() } -> std::same_as<Index<T>>;
    // Has a valid `operator[]`
    { t[Index<T>{}] } -> std::convertible_to<const Value<T>&>;

    // Has a proper `forAllElements` function
    {
        t.forAllElements([] (Index<T>, const Value<T>&) {})
    } -> std::same_as<void>;
}; // <-- concept LinearContainer

/**
 * @brief Similar to \ref LinearContainer, but requires `T` to provide
 *        write access to its elements
 */
template <typename T>
concept MutableLinearContainer = requires (T t) {
    // Has a valid `size()` member function
    { t.getSize() } -> std::same_as<Index<T>>;
    // Has a valid `operator[]`
    { t[Index<T>{}] } -> std::same_as<Value<T>&>;

    // Has a proper `forAllElements` function
    {
        t.forAllElements( [] (Index<T>, Value<T>&) {})
    } -> std::same_as<void>;
}; // <-- concept MutableLinearContainer

/// @brief Operator overloads and extensions
namespace op {

    /**
     * @brief Initialize an array or view with a returning functor
     *
     * `ReturnValue f` must satisfy the following conditions:
     * 1. Accept one argument of type `ArrayLike::IndexType`
     * 2. Return the value of type `ArrayLike::RealType`
     *
     * @return \par av
     */
    template <MutableLinearContainer ArrayLike, typename ReturnValue>
    constexpr inline auto& operator<<(ArrayLike& av, ReturnValue&& f)
    requires requires (Index<ArrayLike> idx) {
        { f(idx) } -> std::convertible_to<Value<ArrayLike>>;
    } {
        av.forAllElements(
            [&f] (auto idx, auto& value) { value = f(idx); }
        );

        return av;
    } // <-- auto& operator<<(arrayLike, f)

    /**
     * @brief Initialize an array or view with a functor that returns via
     *        a reference argument
     *
     * This is the same as: `av.forAllElements(f)`
     *
     * `ResultInArgument f` must satisfy the following conditions:
     * 1. Accept two arguments: one of type `ArrayLike::IndexType` and the
     *    other of type `ArrayLike::RealType&` (or
     *    `const ArrayLike::RealType&`)
     * 2. Modify/initialize the value in the second argument
     * 3. Return `void`
     *
     * TNL didn't constrain `forAllElements` in any way so we have to
     * manually check if `f` is the correct function instead of just
     * asking for `av.forAllElements(f)` to be valid
     */
    template <LinearContainer ArrayLike, typename ResultInArgument>
    constexpr inline auto& operator<<(ArrayLike& av, ResultInArgument&& f)
    requires std::invocable<
        ResultInArgument, Index<ArrayLike>, Value<ArrayLike>&
    > {
        av.forAllElements(f);
        return av;
    } // <-- auto& operator<<(av, f)

} // <-- namespace op

} // <-- namespace noa::utils::tnl
