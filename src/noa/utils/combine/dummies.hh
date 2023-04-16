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

#include <optional>

#include "concepts_prelude.hh"

namespace noa::utils::combine {

template <CTask...> struct DependencyList;

namespace detail {

    /// \brief A dummy variadic template type
    template <typename... Args> struct DummyT {
        /// \brief Create another template instance with the same template arguments
        template <template <typename...> class Template>
        using Transform = Template<Args...>;
    };

    /// \brief Dummy computation implementation
    ///
    /// Every computation must have:
    /// * A template `get()` method for getting a specified task reference (with a `const` overload)
    template <CTask... TaskList> struct DummyComputation {
        template <CTask Task>
        Task& get();

        template <CTask Task>
        const Task& get() const;
    }; // <-- struct DummyComputation

    /// \brief Dummy task implementation
    ///
    /// Defined in task_dynamic.hh
    struct DummyTask;

} // <-- namespace detail

} // <-- namespace noa::utils::detail
