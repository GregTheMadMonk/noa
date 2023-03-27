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

    /// \brief Each time CFDProblem is used, it performs a validity check
    void run(const combine::ComputationType auto& comp) throw(InvalidSetup) {
        if (!this->valid()) throw InvalidSetup{};
    } // <-- void CFDProblem::run()

    /// \brief Set up system layers
    void setup() {
        // Clear the existing layers
        if (!this->domain.hasLayers()) {
        }
    }

private:
    /// \brief Validate CFD problem formulation
    [[nodiscard]] bool valid() {
        // Check the domain is clean
        if (this->domain.isClean()) {
            std::cerr << "The this->domain is empty!" << std::endl;
            return false;
        }

        // Check that all boundaries have at least one boundary condition associated with them
        bool ret = true;
        this->domain.getMesh().template forBoundary<dimEdge>(
            [&this->domain, &ret] (auto edge) {
            }
        );

        if (!ret) {
            std::cerr << "Domain boundary conditions missing on one or more boundary edges!";
        }

        return ret;
    } // <-- bool CFDProblem::valid()
}; // <-- struct CFDProblem

} // <-- namespace noa::utils::cfd::tasks
