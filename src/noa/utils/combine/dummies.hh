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
 * \file dummies.hh
 * \brief Dummy classes
 *
 * This file defines model classes for Computation and Task that
 * serve no purpose other than to act like a placeholder it templated
 * code and SFINAE
 */

#pragma once

namespace noa::utils::combine {

template <typename...> struct DependencyList;

namespace detail {

    /// \brief A dummy variadic template type
    template <typename... Args> struct DummyT {
        /// \brief Create another template instance with the same template arguments
        template <template <typename...> class Template>
        using Transform = Template<Args...>;
    };

    template <template <typename...> class To, template <typename...> class Template, typename... Args>
    To<Args...> vConvert(Template<Args...>);

    /// \brief Dummy computation implementation
    ///
    /// Every computation must have:
    /// * A template `get()` method for getting a specified task reference (with a `const` overload)
    template <typename... TaskList> struct DummyComputation {
        template <typename Task>
        Task& get();

        template <typename Task>
        const Task& get() const;
    }; // <-- struct DummyComputation

    /// \brief Dummy task implementation
    ///
    /// To be a task, a class should have:
    /// * A `Depends` subtype
    struct DummyTask {
        using Depends = DependencyList<>;

        template <typename Computation>
        void run(Computation); // No implementation needed or is provided for a dummy example
    }; // <-- struct DummyTask

} // <-- namespace detail

/// \brief Convert one variadic template into another
///
/// \tparam To target variadic template
/// \tparam Who type to convert
template <template <typename...> class To, typename Who>
using VConvert = decltype(detail::vConvert<To>(std::declval<Who>()));

} // <-- namespace noa::utils::detail
