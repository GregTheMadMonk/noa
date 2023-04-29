/**
 * \file cfd.hh
 * \brief Main file for NOA CFD module
 *
 * Includes all other CFD files
 */

#include "methods.hh"
#include "tasks/cfd_problem.hh"
#include "tasks/mhfe.hh"
#include "tasks/gradev.hh"

/// \brief Namespace for everything related to CFD
namespace noa::utils::cfd {

/// \brief CFD tasks definitions
namespace tasks {}

/// \brief CFD solution methods
namespace methods {}

} // <-- namespace noa::utils::cfd
