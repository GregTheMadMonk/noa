/**
 * \file common_meta.hh
 * \brief Common utility code for metaprogramming/working with templates
 */

#pragma once

#include <concepts>
#include <utility>

/// \brief Namespace with utility code for  metaprogramming/working with templates
namespace noa::utils::meta {

namespace detail {

    #warning "This is a workaround for a g++ bug that doesn't allow us to use a lambda in GetSingleArg"
    template <
        template <typename> class Template,
        typename Argument
    > Argument getSingleArg(Template<Argument>);

} // <-- namespace detail

/// \brief Get a single template argument
template <typename T>
using GetSingleArg = decltype(
    detail::getSingleArg(std::declval<T>())
); // <-- using GetSingleArg

/// \brief Compile-time testing
namespace test {

    struct P1 {};
    template <typename> struct S {};
    static_assert(std::same_as<GetSingleArg<S<P1>>, P1>, "GetSingleArg test failed");

    template <typename T, typename P>
    concept TestGetSingleArg = requires (GetSingleArg<T> t) {
        { t } -> std::same_as<P&>;
    };
    static_assert(TestGetSingleArg<S<P1>, P1>, "GetSingleArg test concept failed");
};

} // <-- namespace noa::utils::meta
