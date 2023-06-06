/**
 * \file gradev.hh
 * \brief Gradient evolution sensitivity calculation method for LMHFE
 */

#pragma once

#include <noa/utils/combine/combine.hh>
#include <noa/utils/domain/domain.hh>

#include "../scalar_function.hh"

namespace noa::utils::cfd::tasks {

/// \brief Gradient evolution sensitivity calculation method
template <auto scalarWrtP, domain::CDomain DomainType>
requires CScalarFuncWrtP<decltype(scalarWrtP), DomainType>
struct GradEv; // Incomplete

/// \brief Specification for triangular domain
///
/// TODO: Could this be generalized for all 2D cases?
template <auto scalarWrtP, domain::CDomainWithTopology<TNL::Meshes::Topologies::Triangle> DomainType>
requires CScalarFuncWrtP<decltype(scalarWrtP), DomainType>
struct GradEv<scalarWrtP, DomainType> : public combine::MakeDynamic<GradEv<scalarWrtP, DomainType>>
{
    /// \brief MHFE task type
    ///
    /// useLumping is `true` because the method is only derived for lumped MHFE
    using LMHFEType        = MHFE<DomainType, true>;

    using ProblemType      = LMHFEType::ProblemType;
    using RealType         = LMHFEType::RealType;
    using LocalIndexType   = LMHFEType::LocalIndexType;
    using GlobalIndexType  = LMHFEType::GlobalIndexType;
    using SparseMatrixType = LMHFEType::SparseMatrixType;

private:
    /// \brief Scalar function pd wrt solution
    ProblemType::RealLayerView gWrtP;
    /// \brief LMHFE system matrix RHS vector pd wrt edge solution (diagonal matrix)
    ProblemType::RealLayerView bWrtTp;
    /// \brief LMHFE system matrix pd wrt a
    std::vector<SparseMatrixType> MWrtA;
    /// \brief GradEv systems RHS vectors
    std::vector<typename ProblemType::RealLayerView> rhs;
    /// \brief A buffer for calculating edge solution derivative wrt to a element
    ProblemType::RealLayerView tpWrtAi;

public:
    /// \brief Default constructor
    ///
    /// Requests resources from the problem
    explicit GradEv(ProblemType& problem, const LMHFEType& lmhfe) {
        problem.requestLayer(DomainType::dimCell, this->gWrtP,   RealType{});
        problem.requestLayer(DomainType::dimEdge, this->bWrtTp,  RealType{});
        problem.requestLayer(DomainType::dimEdge, this->tpWrtAi, RealType{});
    } // <-- GradEv()

    GradEv(const GradEv&) = delete;
    GradEv& operator=(const GradEv&) = delete;
    GradEv(GradEv&&) = default;
    GradEv& operator=(GradEv&&) = default;

    /// \brief Step calculation
    void run(ProblemType& problem, const LMHFEType& lmhfe) {
        if (problem.needsUpdate()) this->update(problem, lmhfe);

        const auto& mesh = problem.getDomain().getMesh();
        const auto cells = mesh.template getEntitiesCount<DomainType::dimCell>();

        scalarWrtP(*problem.solution, *this->gWrtP);
        this->gWrtP.fill();

        this->bWrtTp->forAllElements([&mesh, &problem, &lmhfe, this] (auto edge, auto& v) {
            v = 0;
            if (problem.dirichletMask[edge] && !problem.neumannMask[edge]) return;

            for (LocalIndexType lCell = 0; lCell < lmhfe.localCells[edge]; ++lCell) {
                const auto cell = mesh.template getSuperentityIndex<DomainType::dimEdge, DomainType::dimCell>(edge, lCell);
                v += lmhfe.measures[cell] * problem.c[cell] / (lmhfe.edges[cell] * lmhfe.tau);
            }
        });

        for (GlobalIndexType cell = 0; cell < cells; ++cell) {
            lmhfe.M->forAllElements([&mat = this->MWrtA[cell]] (auto row, auto, auto col, auto&) {
                mat.setElement(row, col, 0);
            });
        }
    } // <-- void GradEv::run()

private:
    void update(ProblemType& problem, const LMHFEType& lmhfe) {
        const auto& mesh = problem.getDomain().getMesh();
        const auto cells = mesh.template getEntitiesCount<DomainType::dimCell>();
        const auto edges = mesh.template getEntitiesCount<DomainType::dimEdge>();

        MWrtA = std::vector<SparseMatrixType>(cells, SparseMatrixType(edges, edges));
        for (auto& MWrtA_e : MWrtA) {
            MWrtA_e.setRowCapacities(*lmhfe.capacities);
        }

        this->rhs = std::vector<typename ProblemType::RealLayerView>(edges);
        for (auto& rhs_e : this->rhs) {
            problem.requestLayer(DomainType::dimEdge, rhs_e, RealType{});
        }
    }
}; // <-- struct GradEv (Triangle)

} // <-- namespace noa::utils::cfd::tasks
