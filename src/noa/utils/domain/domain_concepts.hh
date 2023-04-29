/**
 * \file domain_concepts.hh
 * \brief Domain-related concepts
 */

#pragma once

namespace noa::utils::domain {

//  Forward-declaration of Domain
template <typename, typename, typename, typename, typename>
struct Domain;

/// \brief Concept for all domain types
template <typename DomainCandidate>
concept CDomain = requires (DomainCandidate dc) {
    [] <
        typename CellTopology,
        typename Device,
        typename Real,
        typename GlobalIndex,
        typename LocalIndex
    > (
        Domain<CellTopology, Device, Real, GlobalIndex, LocalIndex>
    ) {} (dc);
}; // <-- concept CDomain

/// \brief Concept for any domain of specific topology
template <typename DomainCandidate, typename CellTopology>
concept CDomainWithTopology = requires(DomainCandidate dc) {
    [] <
        typename Device,
        typename Real,
        typename GlobalIndex,
        typename LocalIndex
    > (
        Domain<CellTopology, Device, Real, GlobalIndex, LocalIndex>
    ) {} (dc);
} && CDomain<DomainCandidate>; // <-- concept CDomainWithTopology

} // <-- namespace noa::utils::domain
