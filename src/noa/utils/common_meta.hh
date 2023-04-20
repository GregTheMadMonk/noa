/**
 * \file common_meta.hh
 * \brief Common utility code for metaprogramming/working with templates
 */

#pragma once

#include <concepts>
#include <tuple>
#include <utility>

/// \brief Namespace with utility code for  metaprogramming/working with templates
namespace noa::utils::meta {

namespace detail {

    template <typename T>
    struct GetSingleArgImpl {};

    template <template <typename> typename T, typename Arg>
    struct GetSingleArgImpl<T<Arg>> { using Type = Arg; };

} // <-- namespace detail

/// \brief Get a single template argument
template <typename T>
using GetSingleArg = detail::GetSingleArgImpl<T>::Type;

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

namespace detail {

    template <typename Return, typename... Args>
    std::tuple<Args...> getArgTypes(Return (*)(Args...));

    template <typename Class, typename Return, typename... Args>
    std::tuple<Args...> getArgTypes(Return (Class::*)(Args...));

    template <typename Class, typename Return, typename... Args>
    std::tuple<Args...> getArgTypes(Return (Class::*)(Args...) const);

    template <typename Functor>
    requires requires {
        { &Functor::operator() };
    } decltype(getArgTypes(&Functor::operator())) getArgTypes(Functor);

} // <-- namespace detail

/// \brief Get argument types of a callable object
///
/// Returns a tuple with argument types equal to types of callable arguments.
///
/// Examples:
/// * `GetArgTypes<&Class::method>`
/// * `GetArgTypes<functor>` (where `functor` is callable via `operator()`)
/// * `GetArgTypes<[] (int, float&&, const char&) {}>` (`std::tuple<int, float&&, const char&>`)
/// * `void f(double) {}; static_assert(std::same_as<GetArgTypes<f>, std::tuple<double>);`
template <auto callable, template <typename...> class To = std::tuple>
using GetArgTypes = decltype(detail::getArgTypes(callable));

namespace detail {

    template <typename Arg>
    struct ArgTag {};

    template <typename Class, std::size_t num>
    struct Tag {
        friend auto loophole(Tag<Class, num>);
    };

    template <typename Class, typename Arg, std::size_t num>
    struct TagDef {
        friend auto loophole(Tag<Class, num>) { return ArgTag<Arg>{}; }
        constexpr friend int cloophole(Tag<Class, num>) { return 0; }
    };

    template <typename Class, typename Arg, std::size_t num>
    requires std::same_as<Class, std::remove_cv_t<std::remove_reference_t<Arg>>> || std::same_as<Class, void>
    struct TagDef<Class, Arg, num> {};

    /// \brief Placeholder structure, implicitly convertible to any type except `Ban`
    ///
    /// \tparam Ban ban casting to this type or its references
    template <std::size_t index = 0, typename Ban = void>
    struct Placeholder {
        using PTag = Tag<Ban, index>;

        template <typename To, std::size_t = sizeof(TagDef<Ban, To, index>)>
        requires (!std::same_as<std::remove_cv_t<std::remove_reference_t<To>>, Ban>)
        constexpr operator To&();
        template <typename To, std::size_t = sizeof(TagDef<Ban, To, index>)>
        requires (!std::same_as<std::remove_cv_t<std::remove_reference_t<To>>, Ban>)
        constexpr operator To&&();
    };

    template <typename Class, typename... Args>
    constexpr std::size_t getConstructorArgsNum() {
        if constexpr (std::constructible_from<Class, Args...>) {
            return sizeof...(Args);
        } else {
            return getConstructorArgsNum<Class, Args..., Placeholder<sizeof...(Args), Class>>();
        }
    }

    template <typename Class, typename... Args>
    struct GetConstructorArgs {
        using Type = GetConstructorArgs<Class, Args..., Placeholder<sizeof...(Args), Class>>::Type;
    };

    template <typename Class, typename... Args>
    requires std::constructible_from<Class, Args...>
    struct GetConstructorArgs<Class, Args...> {
        using Type = std::tuple<GetSingleArg<decltype(loophole(std::declval<typename Args::PTag>()))>...>;
    };

} // <-- namespace detail

/// \brief Get class constructor argument list
///
/// Fetches the arguments of the constructor with the smallest amount of arguments that
/// is not a move- or copy- constructor
///
/// Does not preserve reference types. Does, however, preserve const-qualifiers.
///
/// An implementation inspired by https://stackoverflow.com/a/54493136
template <typename Class>
using GetConstructorArgTypes = detail::GetConstructorArgs<Class>::Type;

namespace test {

    struct TestCArgs {
        TestCArgs(int, const float&, char&&);
    };

    static_assert(
        std::constructible_from<TestCArgs, detail::Placeholder<>, detail::Placeholder<>, detail::Placeholder<>>,
        "Placeholder test failed"
    );

    static_assert(
        detail::getConstructorArgsNum<TestCArgs>() == 3,
        "getConstructorArgsNum test failed"
    );

    static_assert(
        std::same_as<
            detail::GetConstructorArgs<TestCArgs>::Type,
            std::tuple<int, const float, char>
        >, "detail::GetConstructorArgs test failed"
    );

} // <-- namespace test

namespace detail {

    template <template <typename...> class To, template <typename...> class Template, typename... Args>
    To<Args...> vtCast(Template<Args...>);

} // <-- namespace detail

/// \brief Convert one variadic template into another
///
/// \tparam To target variadic template
/// \tparam Who type to convert
template <template <typename...> class To, typename Who>
using VTCast = decltype(detail::vtCast<To>(std::declval<Who>()));

namespace test {

    template <typename...> struct VS {};

    static_assert(
        std::same_as<
            VTCast<std::tuple, VS<int, char, float>>,
            std::tuple<int, char, float>
        >, "VTCast test failed"
    );

} // <-- namespace test

namespace detail {

    template <template <typename> class Who, template <typename...> class Template, typename... Args>
    Template<Who<Args>...> vtApply(Template<Args...>);

} // <-- namespace detail

/// \brief Apply a template to each argument of a variadic template
///
/// If where is `Template<Args...>` this will equate to `Template<Who<Args>...>`
template <template <typename> class Who, typename Where>
using VTApply = decltype(detail::vtApply<Who>(std::declval<Where>()));

namespace test {

    static_assert(
        std::same_as<
            VTApply<std::optional, std::tuple<int, float>>,
            std::tuple<std::optional<int>, std::optional<float>>
        >, "VTApply test failed"
    );

    static_assert(
        std::same_as<
            VTApply<std::remove_reference_t, std::tuple<int&, const float&, char&&>>,
            std::tuple<int, const float, char>
        >, "VTApply with remove_reference_t failed"
    );

} // <-- namespace test

/// \brief Remove references from all template arguments
template <typename Where>
using VTRemoveReferences = VTApply<std::remove_reference_t, Where>;

/// \brief Remove const/volitale qualifiers from all template arguments
template <typename Where>
using VTRemoveCV = VTApply<std::remove_cv_t, Where>;

/// \brief Remove const/volitile qualifiers and refrences from template arguments
template <typename Where>
using VTRemoveCVR = VTRemoveCV<VTRemoveReferences<Where>>;

namespace test {

    static_assert(
        std::same_as<
            VTRemoveCVR<std::tuple<int&, const float&, char&&>>,
            std::tuple<int, float, char>
        >, "VTRemoveCVR test failed"
    );

} // <-- namespace test

/// \brief Checks if the type is an instance of a variadic template
template <typename T, template <typename...> class Template>
concept CVTInstance = requires (T t) {
    {
        [] <typename... Args> (Template<Args...>) -> Template<Args...> {} (t)
    } -> std::same_as<T>;
};

namespace test {

    static_assert(CVTInstance<std::tuple<int, float>, std::tuple>, "CVTInstance test fail");
    static_assert(CVTInstance<GetArgTypes<[] () {}>, std::tuple>, "CVTInstance test fail");

} // <-- namespace test

} // <-- namespace noa::utils::meta
