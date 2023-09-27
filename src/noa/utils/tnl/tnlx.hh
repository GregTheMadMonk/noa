/**
 * @file tnlx.hh
 * @brief Extensions to the TNL library
 */
#pragma once

// Standard library
#include <concepts>

// TNL headers
#include <noa/3rdparty/tnl-noa/src/TNL/Containers/Array.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Matrices/DenseMatrix.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Matrices/SparseMatrix.h>

// NOA headers
#include <noa/utils/common/meta.hh>

/// @brief Extensions to the TNL library
namespace noa::utils::tnl {

namespace detail {
    template <typename Container> struct Index {
        using Type = Container::IndexType;
    }; // <-- struct Index<Container>
}

/// @brief TNL container index type
template <typename Container> using Index
    = detail::Index<Container>::Type;

namespace detail {

    template <typename Container> struct Value {
        using Type = Container::RealType;
    }; // <-- struct Value<Container>

    template <typename Container>
    struct Value<const Container> {
        using Type = const Container::RealType;
    }; // <-- struct Value<const Container>

} // <-- namespace detail

/// @brief TNL container value type
template <typename Container>
using Value = detail::Value<Container>::Type;

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

    /// @brief Initialize an array or view with a value
    template <
        MutableLinearContainer ArrayLike,
        std::convertible_to<Value<ArrayLike>> Real
    > constexpr inline auto& operator<<(ArrayLike& av, Real value) {
        av = value;
        return av;
    } // <-- auto& operator<<(arrayLike, value)

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

/// @brief TNL matrix/matrix view concept
template <typename T>
concept MatrixContainer = T::isMatrix();

/// @brief TNL mutable matrix/matrix view concept
template <typename T>
concept MutableMatrixContainer = requires (T t) {
    requires MatrixContainer<T>;
    {
        t.forAllElements([] (auto, auto, auto, Value<T>& v) {})
    } -> std::same_as<void>;
}; // <-- concept MutableMatrixContainer<T>

/// @brief Dense TNL matrix/view concept
template <typename T>
concept DenseMatrix = (
    requires (T t) {
        [] <
            typename Real,
            typename Device,
            typename Index,
            TNL::Algorithms::Segments::ElementsOrganization Organization,
            typename Allocator
        > (
            TNL::Matrices::DenseMatrix<
                Real, Device, Index, Organization, Allocator
            >
        ) {} (t);
    } || requires (T t) {
        [] <
            typename Real,
            typename Device,
            typename Index,
            TNL::Algorithms::Segments::ElementsOrganization Organization
        > (
            TNL::Matrices::DenseMatrixView<
                Real, Device, Index, Organization
            >
        ) {} (t);
    }
); // <-- concept DenseMatrix<T>

/// @brief Dense TNL matrix/view concept
template <typename T>
concept SparseMatrix = (
    requires (T t) {
        [] <
            typename Real,
            typename Device,
            typename Index,
            typename MatrixType,
            template <typename, typename, typename> class Segments,
            typename ComputeReal,
            typename RealAllocator,
            typename IndexAllocator
        > (
            const TNL::Matrices::SparseMatrix<
                Real, Device, Index, MatrixType, Segments,
                ComputeReal, RealAllocator, IndexAllocator
            >& // Non-copyable
        ) {} (t);
    } || requires (T t) {
        [] <
            typename Real,
            typename Device,
            typename Index,
            typename MatrixType,
            template <typename, typename> class SegmentsView,
            typename ComputeReal
        > (
            TNL::Matrices::SparseMatrixView<
                Real, Device, Index, MatrixType, SegmentsView, ComputeReal
            >
        ) {} (t);
    }
); // <-- concept SparseMatrix<T>

namespace op {

    /// @brief Initialize the matrix with a value
    template <
        MutableMatrixContainer Matrix,
        std::convertible_to<Value<Matrix>> Real
    > constexpr inline Matrix& operator<<(Matrix& m, Real value) {
        m.forAllElements(
            [value] (auto, auto, auto, auto& v) { v = value; }
        );
        return m;
    } // <-- Matrix& operator<<(m, value)

    /**
     * @brief Initialize the matrix with a function that stores its result
     *        in an argument
     *
     * The `f` should take two arguments of type `Index<Matrix>`, first
     * being the matrix row and the second being the matrix column and
     * return the element value. The third argument of reference type
     * is used to access the element value
     */
    template <MatrixContainer Matrix, typename Func>
    constexpr inline Matrix& operator<<(Matrix& m, Func f)
    requires std::invocable<
        Func, Index<Matrix>, Index<Matrix>, Value<Matrix>&
    > {
        if constexpr (DenseMatrix<Matrix>) {
            m.forAllElements(
                [&f] (auto row, auto col, auto, Value<Matrix>& v) {
                    f(row, col, v);
                }
            );
        } else if constexpr (SparseMatrix<Matrix>) {
            m.forAllElements(
                [&f] (auto row, auto, auto col, Value<Matrix>& v) {
                    f(row, col, v);
                }
            );
        } else {
            static_assert(
                false,
                "`operator<<` undefined for this matrix type"
            );
        }

        return m;
    }

    /**
     * @brief Initialize the matrix with a returning function
     *
     * The `f` should take two arguments of type `Index<Matrix>`, first
     * being the matrix row and the second being the matrix column and
     * return the element value
     */
    template <MutableMatrixContainer Matrix, typename Func>
    constexpr inline Matrix& operator<<(Matrix& m, Func f)
    requires requires (Index<Matrix> row, Index<Matrix> col) {
        { f(row, col) } -> std::convertible_to<Value<Matrix>>;
    } {
        m << [&f] (auto row, auto col, auto& v) { v = f(row, col); };
        return m;
    }

} // <-- namespace op

} // <-- namespace noa::utils::tnl
