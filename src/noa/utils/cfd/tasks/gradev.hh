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

    /// \brief Dense matrix type
    using DenseMatrixType = TNL::Matrices::DenseMatrix<RealType, typename DomainType::DeviceType, GlobalIndexType>;

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
    /// \brief Edge solution derivative wrt a
    DenseMatrixType tpWrtA;
    /// \brief Solution derivative wrt a
    DenseMatrixType pWrtA;

    /// \brief Sensitivity result
    ProblemType::RealLayerView gWrtA;

public:
    /// \brief Default constructor
    ///
    /// Requests resources from the problem
    explicit GradEv(ProblemType& problem, const LMHFEType& lmhfe) {
        problem.requestLayer(DomainType::dimCell, this->gWrtP,   RealType{});
        auto& resultLayer = problem.requestLayer(DomainType::dimCell, this->gWrtA,   RealType{});
        resultLayer.alias = "Scalar WRT a";
        resultLayer.exportHint = true;

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
        const auto edges = mesh.template getEntitiesCount<DomainType::dimEdge>();

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

            for (LocalIndexType lEdge1 = 0; lEdge1 < lmhfe.edges[cell]; ++lEdge1) {
                const auto edge1 = mesh.template getSubentityIndex<DomainType::dimCell, DomainType::dimEdge>(cell, lEdge1);
                if (problem.dirichletMask[edge1]) continue;

                for (LocalIndexType lEdge2 = 0; lEdge2 < lmhfe.edges[cell]; ++lEdge2) {
                    const auto edge2 = mesh.template getSubentityIndex<DomainType::dimCell, DomainType::dimEdge>(cell, lEdge2);
                    const auto BinvIndex = (cell * lmhfe.maxEdges + lEdge1) * lmhfe.maxEdges + lEdge2;

                    const auto delta = lmhfe.Binv[BinvIndex] - lmhfe.alpha_i[cell] * lmhfe.alpha_i[cell] / lmhfe.alpha[cell];
                    this->MWrtA[cell].addElement(edge1, edge2, delta, 1);
                }
            }
        }

        for (GlobalIndexType cell = 0; cell < cells; ++cell) {
            this->MWrtA[cell].vectorProduct(*problem.edgeSolution, *this->rhs[cell]);

            this->rhs[cell]->forAllElements([cell, &problem, this] (auto edge, auto& v) {
                if (problem.dirichletMask[edge]) v = 0;
                else v = -v + this->bWrtTp[edge] * this->tpWrtA.getElement(edge, cell);
            });

            if (TNL::lpNorm(*this->rhs[cell], 2.0) <= std::numeric_limits<RealType>::epsilon()) {
                *this->rhs[cell] = 0;
            }
        }

        // Solve the system
        auto preconditioner = TNL::Solvers::getPreconditioner<SparseMatrixType>(lmhfe.preconditionerName);
        auto solver = TNL::Solvers::getLinearSolver<SparseMatrixType>(lmhfe.solverName);

        preconditioner->update(lmhfe.M);
        solver->setMatrix(lmhfe.M);

        for (GlobalIndexType cell = 0; cell < cells; ++cell) {
            solver->solve(*this->rhs[cell], *this->tpWrtAi);
            for (GlobalIndexType edge = 0; edge < edges; ++edge) {
                this->tpWrtA.setElement(edge, cell, this->tpWrtAi[edge]);
                if (std::isnan(this->tpWrtAi[edge])) {
                    throw std::runtime_error("Hit NaN");
                }
            }
        }

        this->pWrtA.forAllElements([&mesh, &problem, &lmhfe, this] (auto cell, auto aCell, auto, auto& v) {
            // V * p_wrt_a
            v *= lmhfe.lambda[cell] / lmhfe.beta[cell];

            // + V_wrt_a * p
            if (cell == aCell) v -= lmhfe.pPrev[cell] * lmhfe.lambda[cell] * lmhfe.alpha[cell] / lmhfe.beta[cell] / lmhfe.beta[cell];

            for (LocalIndexType lEdge = 0; lEdge < lmhfe.edges[cell]; ++lEdge) {
                const auto edge = mesh.template getSubentityIndex<DomainType::dimCell, DomainType::dimEdge>(cell, lEdge);

                // U * tp_wrt_a
                v += problem.a[cell] * this->tpWrtA.getElement(edge, aCell) * lmhfe.alpha_i[cell] / lmhfe.beta[cell];

                // + U_wrt_a * tp
                if (cell == aCell) v += problem.edgeSolution[edge] * lmhfe.alpha_i[cell] * lmhfe.lambda[cell] / lmhfe.beta[cell] / lmhfe.beta[cell];
            }
        });

        // tpWrtAi is a temp buffer, we can safely override it with the result
        this->pWrtA.vectorProduct(*this->gWrtP, *this->gWrtA);
    } // <-- void GradEv::run()

    /// \brief Get const view to the result
    auto getResult() const { return this->gWrtA.getConstView(); }

private:
    void update(ProblemType& problem, const LMHFEType& lmhfe) {
        // TODO Free resources. Not relevant now, need to get the result ASAP
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

        tpWrtA = DenseMatrixType(edges, edges);
        pWrtA  = DenseMatrixType(cells, cells);

        tpWrtA.forAllElements([] (auto, auto, auto, auto& v) { v = 0; });
        pWrtA.forAllElements([] (auto, auto, auto, auto& v) { v = 0; });
    }
}; // <-- struct GradEv (Triangle)

} // <-- namespace noa::utils::cfd::tasks
