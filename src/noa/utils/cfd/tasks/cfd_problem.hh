/**
 * \file cfd_problem.hh
 * \brief Definition of the initial task for CFD problems
 */

#pragma once

#include <noa/utils/combine/combine.hh>
#include <noa/utils/domain/domain.hh>

namespace noa::utils::cfd::tasks {

/**
 * \brief Initial task for all CFD problems
 *
 * Holds the domain with initial conditions/solution layers.
 * Serves as a base for all other CFD computations.
 *
 * `run()` method does nothing
 */
template <domain::CDomain DomainType>
struct CFDProblem : public combine::MakeDynamic<CFDProblem<DomainType>> {
    /// \brief Initial problem. Doesn't depend on anything
    using Depends = combine::Nodeps;

    /// \brief Computation domain
    DomainType domain;

    /// \brief Does nothing
    void run(const combine::ComputationType auto& comp) {}
}; // <-- struct CFDProblem

} // <-- namespace noa::utils::cfd::tasks
