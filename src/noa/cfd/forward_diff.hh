/**
 * @file forward_diff.hh
 * @brief Forward mode differentiation for LMHFE
 */
#pragma once

// Standard library
#include <concepts>

// NOA headers
#include <noa/utils/combine/combine.hh>
#include <noa/utils/tnl/tnlx.hh>

// Local headers
#include "mhfe.hh"

namespace noa::cfd {

/// @brief Forward mode differentiation of LMHFE
template <utils::meta::InstanceOf<utils::domain::Domain> Domain>
struct ForwardDiff; // Incomplete

/// @brief Forward mode differentiation of LMHFE. For triangular meshes
template <utils::meta::InstanceOf<utils::domain::Domain> Domain>
requires std::same_as<
    utils::meta::At0<Domain>, TNL::Meshes::Topologies::Triangle
> struct ForwardDiff<Domain> {
    /// @brief LMHFE task type
    using LMHFE = MHFE<Domain, true>;
    /// @brief Problem task type
    using Problem = LMHFE::Problem;

    using Real        = Problem::Real;
    using Device      = Problem::Device;
    using GlobalIndex = Problem::GlobalIndex;
    using LocalIndex  = Problem::LocalIndex;
    template <typename DataType>
    using Vector = Domain::template Vector<DataType>;

    using DenseMatrix = TNL::Matrices::DenseMatrix<
        Real, Device, GlobalIndex
    >;

    /**
     * @brief Scalar function sensitivity type
     *
     * `std::function` is necessary here to encapsulate different types
     * of functors. Thankfully, it is only evaluated once so the runtime
     * cost of such wrapping is not expected to have an observable effect
     *
     * The second argument is not required to have the same size as the
     * first one: its capacity might exceed it
     */
    using FuncDerivative = std::function<
        void(
            typename Vector<Real>::ConstViewType,
            typename Vector<Real>::ViewType
        )
    >;

    /// @brief A helper buffer over edge dimension
    Problem::template LayerView<Real> edgeBufferLayer;

    /// @brief Calculation result
    Problem::template LayerView<Real> result;
    /// @brief Buffer for scalar function PDs
    Problem::template LayerView<Real> gWrtX;
    /**
     * @brief LMHFE system RHS vector PD wrt edge solution
     *
     * The matrix is diagonal so one vector is sufficient for its storage
     */
    Problem::template LayerView<Real> rhsWrtEdgeSol;
    /// @brief LMHFE system matrix derivative wrt `a`
    std::vector<typename LMHFE::SparseMatrix> MWrtA;
    /// @brief Forward mode diff systems RHS vectors
    std::vector<Vector<Real>> rhs;
    /// @brief Edge solution derivative wrt `a`
    DenseMatrix edgeSolWrtA;
    /// @brief Solution derivative wrt `a`
    DenseMatrix solWrtA;

    /// @brief Cache the cachable values
    void cache(const Problem& prob, const LMHFE& lmhfe) {
        const auto& mesh = prob.getDomain().getMesh();
        const auto cells =
            mesh.template getEntitiesCount<Domain::dCell>();
        const auto edges =
            mesh.template getEntitiesCount<Domain::dEdge>();
        using namespace utils::tnl::op;

        this->MWrtA = std::vector<typename LMHFE::SparseMatrix>(
            cells, typename LMHFE::SparseMatrix(edges, edges)
        );

        for (GlobalIndex cell = 0; cell < cells; ++cell) {
            this->MWrtA[cell].setRowCapacities(lmhfe.getCapacities());
            lmhfe.getM() << [cell, this] (auto row, auto col, auto) {
                this->MWrtA[cell].setElement(row, col, 0);
            };

            for (LocalIndex lei1 = 0; lei1 < lmhfe.getEdges()[cell]; ++lei1) {
                const auto edge1 =
                    mesh.template getSubentityIndex<
                        Domain::dCell, Domain::dEdge
                    >(cell, lei1);

                if (prob.dirichletMask[edge1]) continue;

                for (LocalIndex lei2 = 0; lei2 < lmhfe.getEdges()[cell]; ++lei2) {
                    const auto edge2 =
                        mesh.template getSubentityIndex<
                            Domain::dCell, Domain::dEdge
                        >(cell, lei2);

                    const auto BinvIndex =
                        (cell * lmhfe.getMaxEdges() + lei1) * lmhfe.getMaxEdges()
                        + lei2;

                    this->MWrtA[cell].addElement(
                        edge1, edge2,
                        lmhfe.getBinv()[BinvIndex] - (
                            lmhfe.getAlphaI()[cell] * lmhfe.getAlphaI()[cell]
                            / lmhfe.getAlpha()[cell]
                        ), 1
                    );
                }
            }
        }

        this->rhs =
            std::vector<Vector<Real>>(edges);
        for (auto& rhse : rhs) rhse = Vector<Real>(edges);

        this->edgeSolWrtA = DenseMatrix(edges, edges);
        this->solWrtA = DenseMatrix(cells, cells);

        this->edgeSolWrtA << 0;
        this->solWrtA << 0;

        *this->rhsWrtEdgeSol <<
            [&mesh, &prob, &lmhfe, this] (auto edge, auto& v) {
                v = 0;
                if (prob.dirichletMask[edge] && !prob.neumannMask[edge]) {
                    return;
                }

                for (LocalIndex cli = 0; cli < lmhfe.getLocalCells()[edge]; ++cli) {
                    const auto cell =
                        mesh.template getSuperentityIndex<
                            Domain::dEdge, Domain::dCell
                        >(edge, cli);

                    v +=
                        lmhfe.getMeasures()[cell] * prob.c[cell]
                        / lmhfe.getEdges()[cell] / prob.getTau();
                }
            };
    } // <-- void cache(prob, lmhfe)

public:
    static constexpr auto name = 
        std::same_as<Real, double> ? "dbl_FwMode_LMHFE" : "FwMode_LMHFE";

    /**
     * @brief Scalar function sensitivity with respect to solution
     *
     * Takes the current solution view as the first argument and fills the
     * second one with cell-wise partial derivative of the scalar function
     * in question to it: `δg/δP`
     */
    FuncDerivative scalarWrtSol;

    /**
     * \brief Scalar function sensitivity with respect to `a`
     *
     * Same as \ref scalarWrtSol, but has only an out argument and
     * calculates `δg/δa`
     */
    std::function<void(typename Vector<Real>::ViewType)> scalarWrtA;

    /// @brief Constructor
    ForwardDiff(Problem& prob, const LMHFE& lmhfe)
    : edgeBufferLayer(prob.template addLayer<Real>(Domain::dEdge))
    , result(prob.template addLayer<Real>(Domain::dCell, "Forward mode for LMHFE"))
    , gWrtX(prob.template addLayer<Real>(Domain::dCell))
    , rhsWrtEdgeSol(prob.template addLayer<Real>(Domain::dEdge))
    , scalarWrtSol([] (auto, auto out) { out = 0; })
    , scalarWrtA([] (auto out) { out = 0; })
    { this->cache(prob, lmhfe); }

    /// @brief Calculation step
    void run(const Problem& prob, const LMHFE& lmhfe) {
        const auto& mesh = prob.getDomain().getMesh();
        const auto cells =
            mesh.template getEntitiesCount<Domain::dCell>();
        const auto edges =
            mesh.template getEntitiesCount<Domain::dEdge>();

        using namespace utils::tnl::op;

        for (GlobalIndex cell = 0; cell < cells; ++cell) {
            this->MWrtA[cell].vectorProduct(
                lmhfe.getEdgeSolution(), this->rhs[cell]
            );

            this->rhs[cell] << [cell, &prob, this] (auto edge, auto& v) {
                if (prob.dirichletMask[edge]) v = 0;
                else {
                    v =
                        -v + (
                            this->rhsWrtEdgeSol[edge]
                            * this->edgeSolWrtA.getElement(edge, cell)
                        );
                }
            };

            constexpr auto eps = std::numeric_limits<Real>::epsilon();
            if (TNL::lpNorm(this->rhs[cell], 2.0) <= eps) {
                this->rhs[cell] << 0;
            }

            lmhfe.solve(this->rhs[cell], *this->edgeBufferLayer);
            for (GlobalIndex edge = 0; edge < edges; ++edge) {
                this->edgeSolWrtA.setElement(
                    edge, cell, this->edgeBufferLayer[edge]
                );
            }
        }

        this->solWrtA <<
            [&mesh, &prob, &lmhfe, this] (auto aCell, auto cell, auto& v) {
                // V * p_wrt_a
                v *= lmhfe.getLambda()[cell] / lmhfe.getBeta()[cell];

                // + V_wrt_a * p
                if (cell == aCell) {
                    v -=
                        lmhfe.getPreviousStepSolution()[cell]
                        * lmhfe.getLambda()[cell] * lmhfe.getAlpha()[cell]
                        / lmhfe.getBeta()[cell] / lmhfe.getBeta()[cell];
                }

                for (LocalIndex lei = 0; lei < lmhfe.getEdges()[cell]; ++lei) {
                    const auto edge =
                        mesh.template getSubentityIndex<
                            Domain::dCell, Domain::dEdge
                        >(cell, lei);

                    // U * tp_wrt_a
                    v +=
                        prob.a[cell]
                        * this->edgeSolWrtA.getElement(edge, aCell)
                        * lmhfe.getAlphaI()[cell] / lmhfe.getBeta()[cell];

                    // + U_wrt_a * tp
                    if (cell == aCell) {
                        v +=
                            lmhfe.getEdgeSolution()[edge]
                            * lmhfe.getAlphaI()[cell] * lmhfe.getLambda()[cell]
                            / lmhfe.getBeta()[cell] / lmhfe.getBeta()[cell];
                    }
                }
            };

        this->scalarWrtSol(lmhfe.getSolution(), *this->gWrtX);
        this->solWrtA.vectorProduct(*this->gWrtX, *this->result);
        this->scalarWrtA(*this->gWrtX);

        *this->result << [this] (auto cell, auto& v) {
            v += this->gWrtX[cell];
        };
    }
}; // <-- struct ForwardDiff<Domain> (Triangular)

} // <-- namespace noa::cfd
