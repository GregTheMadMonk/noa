/**
 * \file gradev.hh
 * \brief Gradient evolution sensitivity calculation method for LMHFE
 */

#pragma once

#include <noa/utils/combine/combine.hh>
#include <noa/utils/domain/domain.hh>

namespace noa::utils::cfd::tasks {

/// \brief Gradient evolution sensitivity calculation method
template <domain::CDomain DomainType>
struct GradEv; // Incomplete

/// \brief Specification for triangular domain
///
/// TODO: Could this be generalized for all 2D cases?
template <domain::CDomainWithTopology<TNL::Meshes::Topologies::Triangle> DomainType>
struct GradEv<DomainType> : public combine::MakeDynamic<GradEv<DomainType>> {
    /// \brief MHFE task type
    ///
    /// useLumping is `true` because the method is only derived for lumped MHFE
    using MHFEType = MHFE<DomainType, true>;

    void run(const MHFEType& mhfe) {
    } // <-- void GradEv::run()
}; // <-- struct GradEv (Triangle)

} // <-- namespace noa::utils::cfd::tasks
