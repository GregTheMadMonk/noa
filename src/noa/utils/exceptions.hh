/**
 * \file exceptions.hh
 * \brief NOA exception types
 */

#pragma once

#include <stdexcept>

/// \brief Namesapace for global NOA exception types
namespace noa::utils::errors {

/// \brief FallthroughError occurs when execuion hits a line that's should be unreachable
///        under normal circumstances
struct FallthroughError : public std::runtime_error {
    FallthroughError() : std::runtime_error("Execution hit unreachable point") {}
}; // <-- struct FallthroughError

} // <-- namespace noa::utils::errors
