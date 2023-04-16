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
 * \file concepts_prelude.hh
 * \brief Some of the concepts used in Combine
 *
 * Due to the fact that the concepts cannot be forward-declared,
 * we need to forward-declare their helpers, then define concepts
 * using them, then define helpers in corrseponding headers.
 *
 * We just need this header for some concepts that will end up
 * having a circular dependency otherwise
 *
 * Yikes
 */

#pragma once

#include <noa/utils/common_meta.hh>

namespace noa::utils::combine {

namespace concepts_detail {

    template <typename> constexpr bool constructibleFromComputation = false;

} // <-- namespace concepts_detail

/// \brief Checks if the class is a valid task
template <typename TaskCandidate>
concept CTask = requires {
    requires meta::CVTInstance<meta::GetArgTypes<&TaskCandidate::run>, std::tuple>;
    requires (
        std::default_initializable<TaskCandidate>
        || concepts_detail::constructibleFromComputation<TaskCandidate>
    );
}; // <-- concept CTask

namespace concepts_detail {

    template <typename> constexpr bool dependencyListType = false;

} // <-- namespace concepts_detail

/// \brief Checks if a type is an instance of \ref DependencyList template
template <typename DependencyListCandidate>
concept CDependencyList = concepts_detail::dependencyListType<DependencyListCandidate>;

namespace concepts_detail {

    template <template <typename...> class> constexpr bool hasGet = false;
    template <template <typename...> class> constexpr bool hasConstGet = false;

} // <-- namespace concepts_detail

/// \brief Checks if the class has a valid `get` method for obtaining
///        task references
template <template <typename...> class ComputationCandidate>
concept CHasGet = concepts_detail::hasGet<ComputationCandidate>;

/// \brief Checks if the class has a valid const `get` method for obtaining
///        task references
template <template <typename...> class ComputationCandidate>
concept CHasConstGet = concepts_detail::hasConstGet<ComputationCandidate>;

/// \brief Checks if a template specifies a computation template
template <template <typename...> class ComputationCandidate>
concept CComputationTemplate = requires {
    requires CHasGet<ComputationCandidate>;
    requires CHasConstGet<ComputationCandidate>;
}; // <-- concept CComputationTemplate

/// \brief Checks if a type is an instance of a valid computation template
template <typename ComputationCandidate>
concept CComputation = requires (ComputationCandidate cc) {
    [] <template <typename...> class Template, typename... Ts>
    (Template<Ts...>) requires CComputationTemplate<Template>
    {} (cc);
}; // <-- concept CComputation

} // <-- namespace noa::utils::combine
