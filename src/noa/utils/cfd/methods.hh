/**
 * \file methods.hh
 * \brief MHFE method specifications for CFD problems
 */

#pragma once

#include <noa/utils/common_meta.hh>
#include <noa/utils/domain/domain.hh>

namespace noa::utils::cfd::methods {

/// \brief Information required by the method to calculate the system
template <domain::CDomain DomainType>
struct Args {
    using RealType = DomainType::RealType;

    /// \brief Solution over the cell
    RealType P;
    /// \brief Solution over the edge
    RealType PT;

    /// \brief `a` coefficient over the cell
    RealType a;
    /// \brief `c` coefficient over the cell
    RealType c;
    /// \brief Cell measure
    RealType measure;

    /// \brief Simulation time step value
    RealType tau;

    /// \brief `l` geometric coefficient
    RealType l;

    RealType alpha;
    RealType alpha_i;
    RealType beta;
    RealType lambda;

    RealType bInvEl;
}; // <-- struct Args

/// \brief A concept for valid method types
///
/// A valid method satisfies the following criteria:
/// * Has a DomainType template parameter
/// * Has static `delta`, `lumping` and `rhs` methods to set up linear system
template <typename MethodCandidate>
concept CCFDMethod =
requires (
    meta::GetSingleArg<MethodCandidate> domain,
    Args<meta::GetSingleArg<MethodCandidate>> args
) {
    // Match one template argument that satisfies CDomain
    // (even though it's also checked by Args<>)
    { domain } -> domain::CDomain;
    // Check methods
    { MethodCandidate::delta(args) }   -> std::same_as<typename decltype(domain)::RealType>;
    { MethodCandidate::lumping(args) } -> std::same_as<typename decltype(domain)::RealType>;
    { MethodCandidate::rhs(args) }     -> std::same_as<typename decltype(domain)::RealType>;
}; // <-- concept CCFDMethod

/// \brief MHFE method
template <domain::CDomain> struct MHFE;

/// \brief MHFE implementation for triangular grids
///
/// TODO: Generalize for 2D (?)
template <domain::CDomainWithTopology<TNL::Meshes::Topologies::Triangle> DomainType>
struct MHFE<DomainType> {
    using RealType = DomainType::RealType;

    static RealType delta(const Args<DomainType>& args) {
		return args.a * (args.bInvEl - args.a / args.l / args.l / args.beta);
    } // <-- delta()

    static RealType lumping(const Args<DomainType>& args) {
        return 0;
    } // <-- lumping()

    static RealType rhs(const Args<DomainType>& args) {
		return args.a * args.lambda / args.l / args.beta * args.P;
    } // <-- rhs()
}; // <-- struct MHFE (Triangle)

/// \brief LMHFE method
template <domain::CDomain> struct LMHFE;

/// \brief LMHFE implementation for triangular grids
///
/// TODO: Generalize for 2D (?)
template <domain::CDomainWithTopology<TNL::Meshes::Topologies::Triangle> DomainType>
struct LMHFE<DomainType> {
    using RealType = DomainType::RealType;

    static RealType delta(const Args<DomainType>& args) {
		return args.a * (args.bInvEl - args.alpha_i * args.alpha_i / args.alpha);
    } // <-- delta()

    static RealType lumping(const Args<DomainType>& args) {
		return args.c * args.measure / 3.0 / args.tau;
    } // <-- lumping()

    static RealType rhs(const Args<DomainType>& args) {
		return args.c * args.measure * args.TP / 3.0 / args.tau;
    } // <-- rhs()
}; // <-- struct LMHFE (Triangle)

} // <-- namespace noa::utils::cfd::methods
