/*****************************************************************************
 *   Copyright (c) 2022, Roland Grinis, GrinisRIT ltd.                       *
 *   (roland.grinis@grinisrit.com)                                           *
 *   All rights reserved.                                                    *
 *   See the file COPYING for full copying permissions.                      *
 *                                                                           *
 *   This program is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation, either version 3 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the            *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 *                                                                           *
 *   Implemented by: Gregory Dushkin (yagreg7@gmail.com)                     *
 *****************************************************************************/
/**
 * \file template_details.hh
 * \brief Contains implementation for helpers for working with templated code
 */

#pragma once

#include <type_traits>

namespace noa::utils::combine {

template <typename...> struct DependencyList;

namespace detail {

    /// \brief Determines if a template has a specified type among its variadic arguments
    ///
    /// Returns either `std::true_type` or `std::false_type`
    ///
    /// \tparam Arg argument in question
    /// \tparam Template<Args...> template in question
    template <typename Arg, template <typename...> class Template, typename... Args>
    auto hasVArg(Template<Args...>) {
        if constexpr ((std::is_same_v<Arg, Args> || ...)) {
            return std::true_type{};
        } else {
            return std::false_type{};
        }
    } // <-- hasVArg()

    /// \brief Determine whether a class is a dependency list
    ///
    /// TODO C++20: Replace with a concept
    template <template <typename...> class DepListCandidate, typename... Tasks>
    auto isDepListHelper(DepListCandidate<Tasks...>) {
        if constexpr (std::is_same_v<DepListCandidate<Tasks...>, DependencyList<Tasks...>>) {
            return std::true_type{};
        } else {
            return std::false_type{};
        }
    } // <-- isDepListHelper()

    /// \brief Get last template parameter (helper)
    template <template <typename...> class Template, typename First, typename... Others>
    auto getLastVArg(Template<First, Others...>) {
        if constexpr (sizeof...(Others) == 0) {
            return First{};
        } else {
            return getLastVArg(Template<Others...>());
        }
    }

} // <-- namespace detail

/// \brief DependencyList checker
///
/// Either `std::true_type` or `std::false_type`
///
/// \tparam DependencyListCandidatetype being checked
template <typename DependencyListCandidate>
using IsDependencyList = decltype(detail::isDepListHelper(std::declval<DependencyListCandidate>()));

/// \brief An alias for IsDependencyList<DependencyListCandidate>::value
///
/// \tparam DependencyListCandidatetype being checked
template <typename DependencyListCandidate>
constexpr bool isDependencyList = IsDependencyList<DependencyListCandidate>::value;

namespace detail {
    /// \brief A helper method that helps to join an arbitrary amount of dependency lists
    template <typename... Tasks1, typename... Tasks2>
    DependencyList<Tasks1..., Tasks2...> operator+(DependencyList<Tasks1...>, DependencyList<Tasks2...>);

    /// \brief Dependency list joiner
    ///
    /// The struct is declared inside of a \ref detail namespace in order to have access
    /// to \ref operator+
    template <typename... DependencyLists> struct DepListJoiner {
        static_assert(
            (isDependencyList<DependencyLists> && ...),
            "All DepListJoiner template parameters are expected to be `DependecyList`s"
        );

        using Type = std::remove_reference_t<
            decltype(
                (std::declval<DependencyLists>() + ... + std::declval<DependencyList<>>())
            )
        >;
    };

} // <-- namespace detail

/// \brief DependencyList joiner
///
/// See \ref detail::DepListJoiner
///
/// \tparam DependencyLists an arbitrary number of dependency lists to join
template <typename... DependencyLists>
using DepListsJoin = typename detail::DepListJoiner<DependencyLists...>::Type;

/// \brief Get the last of cless'es template arguments
template <typename T>
using LastVArg = decltype(detail::getLastVArg(std::declval<T>()));

} // <-- namespace noa::utils::combine
