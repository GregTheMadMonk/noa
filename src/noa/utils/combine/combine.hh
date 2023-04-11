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
 * \file combine.hh
 * \brief Combine is NOA's homogeneous compile-/runtime task manager
 *
 * This file includes all headers required to use Combine task management
 * system.
 *
 * ## Combine TODO
 *
 * * **hasRun**: check that the function only accepts a const reference to the computation
 * * Dependency type: **Optionally**
 * * Dependency type: **Either**
 * * Dependency type: **Product**
 */
#pragma once

#include "computation_traits.hh"
#include "dependency_list.hh"
#include "dummies.hh"
#include "dynamic_computation.hh"
#include "static_computation.hh"
#include "task.hh"
#include "task_dynamic.hh"

/**
 * \brief Combine's main namespace
 *
 * Contains all of the Combine library interface and implementation
 */
namespace noa::utils::combine {

/**
 * \brief Detail namespace
 *
 * Namespace designed to hide Combine implementation details
 */
namespace detail {}

} // <-- namespace noa::utils::combine

// Compile-time test
#include "test.hh"
