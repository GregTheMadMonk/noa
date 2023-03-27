/**
 * \file exceptions.hh
 * \brief Exceptions that may arise in CFD calculations
 */

#pragma once

#include <stdexcept>

/// \brief A namespace for CFD exceptions
namespace noa::utils::cfd::errors {

/// \brief Thrown if some conditions expected by solver from the problem aren't met
struct InvalidSetup : public std::runtime_error {
    InvalidSetup() : std::runtime_error("Invalid or missing setup/initial conditions");
}; // <-- struct InvalidSetup

} // <-- namespace noa::utils::cfd::errors
