/**
 * \file meta.hh
 * \brief Common NOA utility code for metaprogramming
 */
#pragma once

#include <cstdint>
#include <concepts>
#include <tuple>
#include <utility>

/// \brief Common NOA utility code for metaprogramming
namespace noa::utils::meta {

/// \brief Structure used for perfect passing of types to functions
template <typename T> struct TypeTag { using Type = T; };

namespace detail {
    template <
        template <typename...> class Template,
        typename... Args
    > void acceptOnlyTemplateInstance(TypeTag< Template<Args...> >) {}
} // <-- namespace detail

/// \brief Checks if the type is an instance of any class template
template <typename T>
concept Template = requires (TypeTag<T> t) {
    detail::acceptOnlyTemplateInstance(t);
}; // <-- concept Template
/// \brief Checks if the type is an instance of a specified class template
template <typename T, template <typename...> class Template>
concept InstanceOf = requires (TypeTag<T> t) {
    detail::acceptOnlyTemplateInstance<Template>(t);
}; // <-- concept InstanceOf

namespace detail::concat {

    template <
        template <typename...> class T1,
        template <typename...> class T2,
        typename... Args1,
        typename... Args2
    > TypeTag< T1<Args1..., Args2...> >
    operator+(TypeTag<T1<Args1...>>, TypeTag<T2<Args2...>>);

    template <Template... Ts>
    struct Concat {
        using Type = decltype( (TypeTag<Ts>{} + ...) )::Type;
    };

    template <>
    struct Concat<> {
        using Type = void;
    };


} // <-- namespace detail::concat

/**
 * \brief 'Concatenate' variadic template instances
 *
 * If `Ts` are instances of different templates, the resulting type has
 * the same template as the first element of `Ts`
 *
 * If `Ts` is empty, `Concat` returns `void`
 */
template <Template... Ts>
using Concat = detail::concat::Concat<Ts...>::Type;

namespace detail::append {

    template <
        template <typename...> class To,
        typename Who,
        typename... Args
    > TypeTag<To<Args..., Who>>
    operator<<(TypeTag<To<Args...>>, TypeTag<Who>);

    template <Template To, typename... Who> using Append =
        decltype( (TypeTag<To>{} << ... << TypeTag<Who>{}) )::Type;

    template <
        template <typename...> class To,
        typename Who,
        typename... Args
    > TypeTag<To<Who, Args...>>
    operator>>(TypeTag<Who>, TypeTag<To<Args...>>);

    template <Template To, typename... Who> using LAppend =
        decltype( (TypeTag<Who>{} >> ... >> TypeTag<To>{}) )::Type;

} // <-- namespace detail::append

/// \brief Append types to the end of a template arguments list
template <Template To, typename... Who>
using Append = detail::append::Append<To, Who...>;

/// \brief Append a type to the start of a template arguments list
template <typename To, typename... Who>
using LAppend = detail::append::LAppend<To, Who...>;

namespace detail {

    template <Template From, std::size_t offset> struct Sub;

} // <-- namespace detail

namespace detail::append_unique {

    template <
        template <typename...> class To,
        typename Who,
        typename... Args
    > TypeTag<
        std::conditional_t<
            (std::same_as<Who, Args> || ... || false),
            To<Args...>, To<Args..., Who>
        >
    > operator<<(TypeTag<To<Args...>>, TypeTag<Who>);
        

    template <Template To, typename... Who> struct AppendUnique;
    template <
        template <typename...> class To,
        typename... Who,
        typename... Args
    > struct AppendUnique<To<Args...>, Who...> {
        using Type =
            decltype(
                (TypeTag<To<Args...>>{} << ... << TypeTag<Who>{})
            )::Type;
    };

    template <
        template <typename...> class To,
        typename Who,
        typename... Args
    > TypeTag<
        std::conditional_t<
            (std::same_as<Who, Args> || ... || false),
            To<Args...>, To<Who, Args...>
        >
    > operator>>(TypeTag<Who>, TypeTag<To<Args...>>);

    template <Template To, typename... Who> struct LAppendUnique;
    template <
        template <typename...> class To,
        typename... Who,
        typename... Args
    > struct LAppendUnique<To<Args...>, Who...> {
        using Type =
            decltype(
                (TypeTag<Who>{} >> ... >> TypeTag<To<Args...>>{})
            )::Type;
    };

} // <-- namespace detail::append_unique

/**
 * \brief Same as \ref Append, but doesn't do anything if the type is
 *        already in the list
 */
template <Template To, typename... Who>
using AppendUnique =
    detail::append_unique::AppendUnique<To, Who...>::Type;

/// \brief Same as \ref AppendUnique, but for \ref LAppend
template <Template To, typename... Who>
using LAppendUnique =
    detail::append_unique::LAppendUnique<To, Who...>::Type;

namespace detail::unique {

    // Right append
    template <
        template <typename...> class Dst,
        typename T,
        typename... Args
    > TypeTag< AppendUnique< Dst<Args...>, T > >
    operator<<(TypeTag< Dst<Args...> >, TypeTag<T>);
    // Left append
    template <
        template <typename...> class Dst,
        typename T,
        typename... Args
    > TypeTag< LAppendUnique< Dst<Args...>, T > >
    operator>>(TypeTag<T>, TypeTag< Dst<Args...> >);

    template <Template Source> struct Unique;
    template <
        template <typename...> class SourceTemplate,
        typename... Args
    > struct Unique<SourceTemplate<Args...>> {
        using Type =
            decltype(
                (TypeTag< SourceTemplate<> >{} << ... << TypeTag<Args>{})
            )::Type;
    };

    template <Template Source> struct LUnique;
    template <
        template <typename...> class SourceTemplate,
        typename... Args
    > struct LUnique<SourceTemplate<Args...>> {
        using Type =
            decltype(
                (TypeTag<Args>{} >> ... >> TypeTag< SourceTemplate<> >{})
            )::Type;
    };

} // <-- namespace detail::unique

/// \brief Leave only the first occurence of each template argument
template <Template Source>
using Unique = detail::unique::Unique<Source>::Type;
/// \brief Leave only the last occurence of each template argument
template <Template Source>
using LUnique = detail::unique::LUnique<Source>::Type;

namespace detail {

    template <Template Subject, template <typename> class Metafunction>
    struct Apply;

    template <
        template <typename...> class SubjectTemplate,
        template <typename> class Metafunction,
        typename... Args
    > struct Apply<SubjectTemplate<Args...>, Metafunction> {
        using Type = SubjectTemplate< Metafunction<Args>... >;
    };

} // <-- namespace detail

/// \brief Apply a metafunction to each of the template's arguments
template <Template Subject, template <typename> class Metafunction>
using Apply = detail::Apply<Subject, Metafunction>::Type;

namespace detail {
    template <typename> struct One { static constexpr auto value = 1; };
    template <Template Subject, template <typename> class Metafunction>
    struct Reduce;

    template <
        template <typename...> class SubjectTemplate,
        template <typename> class Metafunction,
        typename... Args
    > requires requires {
        ( Metafunction<Args>::value + ... );
    } struct Reduce<SubjectTemplate<Args...>, Metafunction> {
        static constexpr auto value = (Metafunction<Args>::value + ...);
    };
} // <-- namespace detail

/// \brief Returns a sum of `::value` members for `Metafunction<Args>...`
template <
    Template Subject,
    template <typename> class Metafunction = detail::One
> constexpr auto reduce = detail::Reduce<Subject, Metafunction>::value;

/// \brief Get template's argument count
template <Template From>
constexpr auto count = reduce<From>;

namespace detail {
    template <Template From, template <typename...> class To>
    struct Caster;

    template <
        template <typename...> class FromTemplate,
        template <typename...> class To,
        typename... Args
    > struct Caster<FromTemplate<Args...>, To> {
        using Type = To<Args...>;
    };
} // <-- namespace detail

/**
 * \brief Use one template's args to instantiate another
 *
 * Converts `From = FromTemplate<Args...>` to `To<Args...>`
 */
template <Template From, template <typename...> class To>
using Cast = detail::Caster<From, To>::Type;

namespace detail {
    template <std::size_t index, typename... Args>
    requires (index < sizeof...(Args))
    struct At;

    template <typename Arg, typename... Args>
    struct At<0, Arg, Args...> {
        using Type = Arg;
    };

    template <std::size_t index, typename Arg, typename... Args>
    struct At<index, Arg, Args...> {
        using Type = At<index - 1, Args...>::Type;
    };

    template <Template From, std::size_t index> struct At1;

    template <
        std::size_t index,
        template <typename...> class FromTemplate,
        typename... Args
    > struct At1<FromTemplate<Args...>, index> {
        using Type = At<index, Args...>::Type;
    };
} // <-- namespace detail

/// \brief Get template's argument at position
template <Template From, std::size_t index>
using At = detail::At1<From, index>::Type;

/// \brief Get first (or single) template's argument
template <Template From>
using At0 = At<From, 0>;

namespace detail {

    template <typename Indices, Template From, std::size_t offset>
    struct CropImpl;

    template <
        template <typename T1, T1...> class IndexTemplate,
        template <typename...> class FromTemplate,
        std::size_t offset,
        typename... Args,
        typename Index,
        Index... indices
    > struct CropImpl <
        IndexTemplate<Index, indices...>, FromTemplate<Args...>, offset
    > {
        using From = FromTemplate<Args...>;
        using Type =
            FromTemplate<noa::utils::meta::At<From, indices + offset>...>;
    }; // <-- struct CropImpl

    template <Template From, std::size_t idx>
    requires (idx <= count<From>)
    struct Crop {
        using Indices =
            std::make_integer_sequence<std::size_t, count<From> - idx>;
        using Type = CropImpl<Indices, From, idx>::Type;
    }; // <-- struct Crop

    template <Template From, std::size_t idx>
    requires (idx <= count<From>)
    struct LCrop {
        using Indices =
            std::make_integer_sequence<std::size_t, count<From> - idx>;
        using Type = CropImpl<Indices, From, 0>::Type;
    }; // <-- struct Crop

} // <-- namespace detail

/// \brief Crop first `offset` arguments from the template
template <Template From, std::size_t offset>
using Crop = detail::Crop<From, offset>::Type;

/// \brief Crop last `offset` arguments from the template
template <Template From, std::size_t offset>
using LCrop = detail::LCrop<From, offset>::Type;

/**
 * \brief Dummy type list
 *
 * Main designation: storing lists of types without the overhead of
 * `std::tuple` instantiation.
 */
template <typename... Ts> struct List {
    /// \brief Instantiate another template from List arguments
    template <template <typename...> class Other>
    using To = Other<Ts...>;

    /// \brief Append types to the end of the list
    template <typename... Elements>
    using Append = List<Ts..., Elements...>;
    /// \brief Append types to the start of the list
    template <typename... Elements>
    using LAppend = List<Elements..., Ts...>;

    /// \brief See \ref noa::utils::meta::Unique
    using Unique = noa::utils::meta::Unique<List>;
    /// \brief See \ref noa::utils::meta::LUnique
    using LUnique = noa::utils::meta::LUnique<List>;

    /// \brief See \ref noa::utils::meta::Apply
    template <template <typename> class Metafunction>
    using Apply = noa::utils::meta::Apply<List, Metafunction>;

    /// \brief See \ref noa::utils::meta::Crop
    template <std::size_t offset>
    using Crop = noa::utils::meta::Crop<List, offset>;

    /// \brief See \ref noa::utils::meta::LCrop
    template <std::size_t offset>
    using LCrop = noa::utils::meta::LCrop<List, offset>;

    /// \brief See \ref noa::utils::meta::reduce
    template <template <typename> class Metafunction>
    static constexpr auto reduce =
        noa::utils::meta::reduce<List, Metafunction>;

    /// \brief List size
    static constexpr std::size_t size = sizeof...(Ts);

    /// \brief Is the list empty?
    static constexpr bool empty = (size == 0);

    /// \brief Returns the amonut of times a type is present in `Ts`
    template <typename Element>
    static constexpr std::size_t count =
        (std::same_as<Element, Ts> + ... + 0);

    /// \brief Checks if the list contains a type
    template <typename Element>
    static constexpr bool contains = (count<Element> != 0);

    /// \brief Checks if the element is a unique member of the list
    template <typename Element>
    static constexpr bool containsUnique = (count<Element> == 1);
}; // <-- struct List<Ts...>

/// \brief Use template's arguments to create a \ref List
template <Template From>
using MakeList = Cast<From, List>;

namespace detail {

    template <typename Callable>
    struct CallableTypeArgs;

    template <typename Return, typename... Args>
    struct CallableTypeArgs< Return(Args...) > {
        using Type = List<Args...>;
    };

    template <typename Return, typename... Args>
    struct CallableTypeArgs< Return (*)(Args...) > {
        using Type = List<Args...>;
    };

    template <typename Class, typename Return, typename... Args>
    struct CallableTypeArgs< Return (Class::*)(Args...) > {
        using Type = List<Args...>;
    };

    template <typename Class, typename Return, typename... Args>
    struct CallableTypeArgs< Return (Class::*)(Args...) const > {
        using Type = List<Args...>;
    };

    template <typename Callable>
    requires requires { &Callable::operator(); }
    struct CallableTypeArgs<Callable> {
        using Type =
            CallableTypeArgs< decltype(&Callable::operator()) >::Type;
    };

} // <-- namespace detail

/**
 * \brief Get argument types of a callable type
 *
 * \tparam Callable - the callable type
 */
template <typename Callable>
using GetCallableTypeArgs = detail::CallableTypeArgs<Callable>::Type;
/**
 * \brief Get argument types of a callable object
 *
 * \tparam callable - the callable. Could be a function, a member function
 *                    or a functor reference
 */
template <auto callable>
using GetCallableArgs = GetCallableTypeArgs< decltype(callable) >;

/**
 * \brief Namespace with utility code for resolving class constructor
 *        argument types
 */
namespace detail::class_construct {

    #ifndef NOA_META_MAX_CONSTRUCTOR_ARGS
    #define NOA_META_MAX_CONSTRUCTOR_ARGS 64
    #endif
    /**
     * \brief Maxmum number of detectable constructor argument types
     *
     * Customizable via presettings the NOA_META_MAX_CONSTRUCTOR_ARGS
     * macro before including the file
     */
    constexpr std::size_t maxDetectableArgs =
        NOA_META_MAX_CONSTRUCTOR_ARGS;

    /*
     * Ok, so, here, unlike any other "detail" namespaces, I will leave
     * detailed (no pun intended) comments about the implementation
     * because, to be completely honest, it is IMO a mess 😐
     * This is a customized solution from StackOverflow (see docstrings
     * for noa::utils::meta::GetConstructorArgs) that I personally have
     * had a hard time understanding some time after the first two times
     * I've implemented it. This is the last, much better and more complex
     * version. Enjoy.
     */

    /*
     * Ulike the StackOverflow solution, there will be no argument tag
     * structure involved. We have TypeTag for this already
     */

    /*
     * Indexed argument tag. `idx` is, naturally, the argument index.
     * `Class` is the class for which this structure is used.
     *
     * `Pfx...` is the most interseting of all arguments: this parameter
     * pack describes the starting sequence of arguments for the currently
     * processed constructor. This is needed to be able to receive more
     * than one constructor signature for the given class without running
     * into multiple definition issues for the loophole function
     */
    template <typename Class, std::size_t idx, typename... Pfx>
    struct Tag {
        /*
         * 🤓 "friend declaration ‘...’ declares a non-template function"
         * I KNOW 😡 I MEANT IT
         */
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wpragmas"
        #pragma GCC diagnostic ignored "-Wunknown-warning-option"
        #pragma GCC diagnostic ignored "-Wnon-template-friend"
        /**
         * The first loophole function declaration
         */
        friend auto loophole(Tag<Class, idx, Pfx...>);
        #pragma GCC diagnostic pop
    }; // <-- struct Tag

    /*
     * A structure that launches instantiation of the `Tag` template and
     * the loophole function. This way it maps index `idx` to argument
     * type `Arg`. `Pfx...` and `Class` have the same designation as
     * previously.
     */
    template <
        typename Class, typename Arg, std::size_t idx, typename... Pfx
    > struct TagDef {
        friend auto loophole(Tag<Class, idx, Pfx...>) {
            return TypeTag<Arg>{};
        }
    }; // <-- struct TagDef

    /*
     * This specialization is required for avoiding the detection of copy
     * and move constructors by default. The avoiding is implemented in
     * the Placeholder struct, but we need to ensure that TagDef for that
     * case does not instantiate any loophole logic
     */
    template <
        typename Class, typename Arg, std::size_t idx, typename... Pfx
    > constexpr bool skipDef = (
        // The first argument + no arguments were provided before it
        idx == 0 && sizeof...(Pfx) == 0
        // and the type is the same as the class type (except cvref stuff)
        && std::same_as< Class, std::remove_cvref_t<Arg> >
    );

    template <
        typename Class, typename Arg, std::size_t idx, typename... Pfx
    > requires skipDef<Class, Arg, idx, Pfx...>
    struct TagDef<Class, Arg, idx, Pfx...>
    { /* No instantiations here! */ };

    /*
     * Now the star of the show - the placeholder class!
     * It converts implictly to any type. Conversion operator launches
     * instantiation of the `TagDef` which, in turn, sets up the loophole
     * (except of the case regulated by the `requires` clause and `TagDef`
     * specification)
     */
    template <typename Class, std::size_t idx, typename... Pfx>
    struct Placeholder {
        using MyTag = Tag<Class, idx, Pfx...>;

        /*
         * Sadly, when the concept condition is not satisfied, the
         * `auto = sizeof(TagDef<...>)` part still gets evaluated.
         * This is why we need to specialize `TagDef` for this case
         * too! (the concept here is inverted from `TagDef`
         * specialization :) ).
         */
        template <
            // Convert to any type `To`
            typename To,
            // Instantiate the needed `TagDef`
            auto = sizeof(TagDef<Class, To, idx, Pfx...>)
        > requires (!skipDef<Class, To, idx, Pfx...>)
        operator To&();

        /**
         * Bind to both rvalues and lvalues, but convert only to
         * references. Otherwise `const` detection is not guaranteed
         */
        template <
            typename To,
            auto = sizeof(TagDef<Class, To, idx, Pfx...>)
        > requires (!skipDef<Class, To, idx, Pfx...>)
        operator To&&();

        /*
         * There _is_, actually, a way to differentiate between T&&, T&
         * and const T& in constructor arguments via these conversions:
         * the last conversion will only happen at places where an
         * rvalue reference argument is needed. BUT this will introduce
         * unnecessary complications and will mimic actual type detection
         * closely enough to introduce confusion: still, `T` and
         * `const T&` are the same argument type for this method. :(
         *
         * At lest, we could detect `const`/non-`const`
         */
    }; // <-- struct Placeholder

    /*
     * Struct for easier loophole access. Expect the loophole to get the
     * argument tag that needs to return the type tag.
     */

    template <typename T>
    requires requires (T t) { loophole(t); }
    struct Loophole {
        using Type = decltype( loophole(std::declval<T>()) )::Type;
    };

    /// Can't have two parameter packs - need to store Pfx in a `List`!
    template <typename Class, InstanceOf<List> Pfx, typename... Args>
    struct GetConstructorArgs;

    /// Regular case
    template < typename Class, typename... Pfx, typename... Args >
    requires (
        sizeof...(Args) <= maxDetectableArgs
        && !std::constructible_from<Class, Pfx..., Args...>
    ) struct GetConstructorArgs<Class, List<Pfx...>, Args...> {
        using Type = GetConstructorArgs<
            Class, List<Pfx...>,
            Args...,
            Placeholder<Class, sizeof...(Args) + sizeof...(Pfx), Pfx...>
        >::Type;
    };

    /// Good ending (successfull termination case)
    template < typename Class, typename... Pfx, typename... Args >
    requires std::constructible_from< Class, Pfx..., Args... >
    struct GetConstructorArgs<Class, List<Pfx...>, Args...> {
        using Type = List<
            Pfx..., typename Loophole< typename Args::MyTag >::Type...
        >;
    };

    /*
     * Argument count exceeds the limit - terminate the search and return
     * `void` for failure :(
     * I don't like the "limit the argument count" approach but I don't
     * see any alternative currently :(
     */
    template <typename Class, InstanceOf<List> Pfx, typename... Args>
    requires (sizeof...(Args) > maxDetectableArgs)
    struct GetConstructorArgs<Class, Pfx, Args...> {
        using Type = void;
    };
} // <-- namespace detail::class_construct

/**
 * \brief Get shortest argument type list for class costructor
 *
 * Fetches agrument types for the constructor with the smallest amount of
 * arguments that is not a move- or copy- constructor
 *
 * Does not preserve reference types due to C++ conversion rules and
 * the method used, but preserves const-qualifiers.
 *
 * Evaluates to `void` if there is no constructor to find.
 *
 * Implementation inspired by https://stackoverflow.com/a/54493136
 *
 * **FIXME**: Currently a workaround for the case when a type without a
 * matching constructor is passed is to artificially limit the maximum
 * detected argument count via
 * \ref detail::class_construct::maxDetectableArgs.
 * I don't like it.
 *
 * \tparam Types... - if specified, search for the constructor that has
 *                    first arguments with types `Types`. This approach
 *                    is pretty unreliable in some cases due to the
 *                    limitations of the method reyling on conversion
 *                    operator instantiation. Use at your own risk.
 */
template <typename Class, typename... Types>
using GetConstructorArgs =
    detail::class_construct::GetConstructorArgs<
        Class, List<Types...>
    >::Type;

#ifdef NOA_COMPILE_TIME_TESTS
/// \brief Compile-time header testing
namespace test {

namespace test1 {

static_assert(
    std::same_as<
        Cast<
            List< List<int, float>, List<char, double>, List<long> >,
            Concat
        >, List< int, float, char, double, long >
    >
);

struct S1 {};
template <typename = void> struct S2 {};
template <typename, typename> struct S3 {};
template <typename...> struct S4 {};

static_assert(!Template<S1>);
static_assert( Template<S2<int>>);
static_assert( Template<S3<int, float>>);
static_assert( Template<S4<int>>);
static_assert( Template<S4<int, float>>);
static_assert( Template<S4<int, float, char>>);

static_assert(!InstanceOf<void, List>);

static_assert( InstanceOf<S2<>, S2>);
static_assert( InstanceOf<S2<int>, S2>);
static_assert( InstanceOf<S3<int, char>, S3>);
static_assert( InstanceOf<S4<>, S4>);
static_assert( InstanceOf<S4<int>, S4>);
static_assert( InstanceOf<S4<int, float>, S4>);
static_assert( InstanceOf<S4<int, char, double>, S4>);

static_assert(std::same_as<Cast<List<>, S4>, S4<>>);
static_assert(
    std::same_as<Cast<List<int, float, char>, S4>, S4<int, float, char>>
);
static_assert(std::same_as<Cast<List<>, S4>, S4<>>);

static_assert(std::same_as<MakeList<S4<int, char>>, List<int, char>>);

using TestList = List<int, int, double, char>;
static_assert(4 == count<TestList>);
static_assert(std::same_as<At0<TestList>, int>);
static_assert(std::same_as<At<TestList, 1>, int>);
static_assert(std::same_as<At<TestList, 2>, double>);
static_assert(std::same_as<At<TestList, 3>, char>);

static_assert(std::same_as<TestList::Crop<2>, List<double, char>>);
static_assert(std::same_as<TestList::LCrop<2>, List<int, int>>);

static_assert(4 == reduce<TestList>);
template <typename T> struct IsInt {
    static constexpr int value = std::same_as<T, int>;
};
static_assert(2 == reduce<TestList, IsInt>);

static_assert(TestList::count<int> == 2);
static_assert(TestList::contains<int>);
static_assert(!TestList::contains<float>);
static_assert(!TestList::containsUnique<int>);
static_assert(TestList::containsUnique<char>);
static_assert(
    std::same_as<
        TestList::Append<char, double>,
        List<int, int, double, char, char, double>
    >
);
static_assert(
    std::same_as<
        TestList::LAppend<char, double>,
        List<char, double, int, int, double, char>
    >
);
static_assert(
    std::same_as<
        Concat<TestList, S4<char, char>, S4<float>>,
        List<int, int, double, char, char, char, float>
    >
);
static_assert(
    std::same_as<
        Apply<TestList, S2>,
        List<S2<int>, S2<int>, S2<double>, S2<char>>
    >
);

using TestList2 = List<int, char, int, int, double, char>;
static_assert(std::same_as<TestList2::Unique, List<int, char, double>>);
static_assert(std::same_as<TestList2::LUnique, List<int, double, char>>);

static constexpr auto l = [] (int, const float&, S1&&) {};
struct T {
    T(S1); T(S2<>, S2<int>); T(S2<int>, const S3<void, void>&);

    void f(char);
    void g(int) const;
};
void u(int, int);

static_assert(std::same_as<GetCallableArgs<u>, List<int, int>>);
static_assert(
    std::same_as<GetCallableTypeArgs<decltype(u)>, List<int, int>>
);
static_assert(std::same_as<GetCallableArgs<&T::f>, List<char>>);
static_assert(std::same_as<GetCallableArgs<&T::g>, List<int>>);
static_assert(
    std::same_as<GetCallableArgs<l>, List<int, const float&, S1&&>>
);

static_assert(std::same_as<GetConstructorArgs<T>, List<S1>>);
static_assert(
    std::same_as<GetConstructorArgs<T, S2<>>, List<S2<>, S2<int>>>
);
static_assert(std::same_as<
    GetConstructorArgs<T, S2<int>>,
    List<S2<int>, const S3<void, void>>
>);

struct U {};
static_assert(std::same_as<GetConstructorArgs<U>, List<>>);

} // <-- namespace test1

} // <-- namespace test
#endif

} // <-- namespace noa::utils::meta
