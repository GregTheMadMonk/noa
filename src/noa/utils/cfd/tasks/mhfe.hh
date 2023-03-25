/**
 * \file mhfe.hh
 * \brief Contains a task for solving a CFD problem with MHFE
 *
 * MHFE = Mixed Hybrid Finite Element method
 */

#pragma once

#include <noa/utils/combine/combine.hh>
#include <noa/utils/domain/domain.hh>

#include "cfd_problem.hh"
#include "../cfd_methods.hh"

namespace noa::utils::cfd::tasks {

/**
 * \brief MHFE task base template
 *
 * Specialized for different domain topologies below.
 * Exact method (with or without lumping) is specified as a Method
 * template parameter
 */
template <domain::CDomain DomainType, MHFEMethod Method>
struct MHFE {};

/**
 * \brief MHFE specialization for triangular meshes
 *
 * TODO: MHFEM implementation could be generalized for all 2D cases
 */
template <domain::CDomainWithTopology<TNL::Meshes::Topologies::Triangle> DomainType>
struct MHFE<DomainType> : public combine::MakeDynamic<MHFE<DomainType>> {
    using Depends = combine::DependencyList<CFDProblem<DomainType>>;

    /// \brief Perform MHFE solution step
    void run(const combine::ComputationType auto& comp) {
    } // <-- void MHFE::run()
}; // <-- struct MHFE (Triangle)

} // <-- namespace noa::utils::cfd::tasks
