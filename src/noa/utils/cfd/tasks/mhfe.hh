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
#include "../methods.hh"

namespace noa::utils::cfd::tasks {

/**
 * \brief MHFE task base template
 *
 * \tparam MethodType method type to be used
 *
 * Specialized for different domain topologies below.
 * Exact method (with or without lumping) is specified as a Method
 * template parameter
 */
template <methods::CCFDMethod MethodType>
struct MHFE; // Incomplete

/**
 * \brief MHFE specialization for triangular meshes
 *
 * TODO: MHFEM implementation could be generalized for all 2D cases
 */
template <methods::CCFDMethod MethodType>
requires
    domain::CDomainWithTopology<
        meta::GetSingleArg<MethodType>,
        TNL::Meshes::Topologies::Triangle
    >
struct MHFE<MethodType> : public combine::MakeDynamic<MHFE<MethodType>> {
    using DomainType = meta::GetSingleArg<MethodType>;

    using Depends = combine::DependencyList<CFDProblem<DomainType>>;

    /// \brief Perform MHFE solution step
    void run(const combine::ComputationType auto& comp) {
    } // <-- void MHFE::run()

private:
}; // <-- struct MHFE (Triangle)

} // <-- namespace noa::utils::cfd::tasks
