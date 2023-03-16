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
 * * C++20 🙂
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
