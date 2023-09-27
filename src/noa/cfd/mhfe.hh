/**
 * @file mhfe.hh
 * @brief MHFE/LMHFE solver
 */
#pragma once

// Standard library
#include <concepts>

// TNL headers
#include <noa/3rdparty/tnl-noa/src/TNL/Solvers/LinearSolverTypeResolver.h>

// NOA headers
#include <noa/utils/combine/combine.hh>
#include <noa/utils/common/meta.hh>
#include <noa/utils/common/unreachable.hh>
#include <noa/utils/domain/domain.hh>
#include <noa/utils/tnl/tnlx.hh>

// Local headers
#include "problem.hh"

// `getEntityMeasure` must be included after `Mesh`
#include <noa/3rdparty/tnl-noa/src/TNL/Meshes/Geometry/getEntityMeasure.h>

namespace noa::cfd {

/**
 * @brief MHFE/LMHFE solver task. Specialized for different domain
 *        topologies
 */
template <
    utils::meta::InstanceOf<utils::domain::Domain> Domain,
    bool lumping
> struct MHFE;

/**
 * @brief MHFE/LMHFE solver specialization for triangular meshes
 *
 * TODO: The implementation could be generalized for arbitrary geometry
 */
template <
    utils::meta::InstanceOf<utils::domain::Domain> Domain,
    bool lumping
> requires std::same_as<
    utils::meta::At0<Domain>, TNL::Meshes::Topologies::Triangle
> struct MHFE<Domain, lumping> {
    /// @brief CFD problem task type
    using Problem = CFDProblem<Domain>;

    template <typename DataType>
    using LayerView = Problem::template LayerView<DataType>;

    using Real         = Domain::RealType;
    using Device       = Domain::DeviceType;
    using GlobalIndex  = Domain::GlobalIndexType;
    using LocalIndex   = Domain::LocalIndexType;

    using SparseMatrix =
        TNL::Matrices::SparseMatrix<Real, Device, GlobalIndex>;

    template <typename DataType>
    using Vector = Problem::template Vector<DataType>;

    /// @brief Task name is MHFE or LMHFE depending on `lumping`
    static constexpr auto name = 
        std::same_as<Real, double>
        ? (lumping ? "dbl_LMHFE" : "dbl_MHFE")
        : (lumping ? "LMHFE" : "MHFE");

private:
    /// @brie Problem solution
    LayerView<Real> solution;
    /// @brief Solution on the previous time step
    LayerView<Real> prevSolution;

    /// @brief Problem edge-wise solution
    LayerView<Real> edgeSolution;

    /// @brief System matrix row capacities
    LayerView<GlobalIndex> capacities;

    /// @brief Number of cells adjacent to an edge
    LayerView<LocalIndex> localCells;
    /// @brief Number of edges per cell
    LayerView<LocalIndex> edges;
    /// @brief Max amount of edges per cell
    LocalIndex maxEdges;
    /// @brief Cached cell measures
    LayerView<Real> measures;

    LayerView<Real> lambda;
    LayerView<Real> alpha_i;
    LayerView<Real> alpha;
    LayerView<Real> beta;
    LayerView<Real> l;

    /// @brief MHFEM system matrix
    std::shared_ptr<SparseMatrix> M;
    /// @brief MHFEM system right-hand-side vector
    LayerView<Real> rhs;

    /// @brief B^{-1} matrix elements
    std::vector<Real> Binv;

    /// @brief Preconditioner name
    std::string preconditionerName = "diagonal";
    /// @brief Linear solver name
    std::string solverName         = "gmres";

    /// @brief TNL preconditioner type
    using Preconditioner = TNL::Solvers::Linear::Preconditioners
                                        ::Preconditioner<SparseMatrix>;
    /// @brief System preconditioner
    std::shared_ptr<Preconditioner> preconditioner = nullptr;
    /// @brief TNL linear solver type
    using Solver = TNL::Solvers::Linear::LinearSolver<SparseMatrix>;
    /// @brief System solver
    std::shared_ptr<Solver> solver = nullptr;

    /// @brief Updated state. Triggered by a recalculation of cached vals
    bool isUpdated = false;
    
    /// @brief System matrix delta from a pair of cell's edges
    Real delta(
        const Problem& prob,
        GlobalIndex cell,
        LocalIndex edge1,
        LocalIndex edge2
    ) {
        const auto BinvIndex =
            (cell * this->maxEdges + edge1) * this->maxEdges + edge2;

        if constexpr (lumping) {
            return
                prob.a[cell] * (
                    this->Binv[BinvIndex] - (
                        this->alpha_i[cell] * this->alpha_i[cell]
                        / this->alpha[cell]
                    )
                ) + (edge1 == edge2) * (
                    prob.c[cell] * this->measures[cell]
                    / 3.0 / prob.getTau()
                );
        } else {
            return
                prob.a[cell] * (
                    this->Binv[BinvIndex] - (
                        prob.a[cell] / this->l[cell] / this->l[cell]
                        / this->beta[cell]
                    )
                );
        }
    } // <-- Real delta(cell, edge1, edge2)

    /**
     * @brief Add contibution from a cell/edge pair to the system matrix
     *
     * \param prob \ref CFDProblem task
     * \param cell Cell global index
     * \param edge Edge global index
     */
    void addMTerm(
        const Problem& prob, GlobalIndex cell, GlobalIndex edge
    ) {
        const auto& mesh = prob.getDomain().getMesh();

        const auto edgeLocal = [&mesh, edge, cell, this] {
            for (LocalIndex lei = 0; lei < this->edges[cell]; ++lei) {
                const auto edge2 =
                    mesh.template getSubentityIndex<
                        Domain::dCell, Domain::dEdge
                    >(cell, lei);

                if (edge == edge2) return lei;
            }

            utils::unreachable();
        } ();

        for (LocalIndex lei = 0; lei < this->edges[cell]; ++lei) {
            const auto edge2 =
                mesh.template getSubentityIndex<
                    Domain::dCell, Domain::dEdge
                >(cell, lei);

            this->M->addElement(
                edge, edge2, this->delta(prob, cell, lei, edgeLocal), 1
            );
        }
    } // <-- void addMTerm(prob, cell, edge)

    /**
     * @brief Calculate parameters that don't need to be updated every
     *        step.
     */
    void cache(const Problem& prob) {
        const auto& domain = prob.getDomain();
        const auto& mesh = prob.getDomain().getMesh();
        const auto cells =
            mesh.template getEntitiesCount<Domain::dCell>();
        const auto edges =
            mesh.template getEntitiesCount<Domain::dEdge>();

        using namespace utils::tnl::op;

        // Fill cells per edge
        *this->localCells << [&mesh] (auto edge) {
            return mesh.template getSuperentitiesCount<
                       Domain::dEdge, Domain::dCell
                   >(edge);
        };

        // Fill edges per cell
        *this->edges << [&mesh] (auto cell) {
            return mesh.template getSubentitiesCount<
                       Domain::dCell, Domain::dEdge
                   >(cell);
        };

        // Fill cell measures
        *this->measures << [&mesh] (auto cell) {
            return TNL::Meshes::getEntityMeasure(
                mesh,
                mesh.template getEntity<Domain::dCell>(cell)
            );
        };

        // Fill cell parameters
        *this->l << [&mesh, this] (auto cell) {
            Real sqSum = 0;

            for (LocalIndex edge = 0; edge < this->edges[cell]; ++edge) {
                const auto gEdgeIdx =
                    mesh.template getSubentityIndex<
                        Domain::dCell, Domain::dEdge
                    >(cell, edge);
                
                const auto p1 = mesh.getPoint(
                    mesh.template getSubentityIndex<
                        Domain::dEdge, 0
                    >(edge, 0)
                );
                const auto p2 = mesh.getPoint(
                    mesh.template getSubentityIndex<
                        Domain::dEdge, 0
                    >(edge, 1)
                );

                const auto r = p2 - p1;
                sqSum += (r, r);
            }

            return sqSum / 48.0 / this->measures[cell];
        };
        *this->lambda << [&prob, this] (auto cell) {
            return prob.c[cell] * this->measures[cell] / prob.getTau();
        };
        *this->alpha_i << [this] (auto cell) {
            return 1.0 / this->l[cell];
        };
        *this->alpha << [this] (auto cell) {
            return this->edges[cell] * this->alpha_i[cell];
        };
        *this->beta << [&prob, this] (auto cell) {
            return this->lambda[cell] + prob.a[cell] * this->alpha[cell];
        };

        this->maxEdges = TNL::Algorithms::reduce(*this->edges, TNL::Max{});

        // Fill B^{-1}
        this->Binv = std::vector<Real>(
            cells * this->maxEdges * this->maxEdges,
            Real{}
        );
        // Binv elements will be stored seuently for each cell
        // |elem|elem|elem|elem|elem|elem| ...
        // |     row      |     row      | ...
        // |                    Binv       ...
        for (GlobalIndex cell = 0; cell < cells; ++cell) {
            // B^{-1} contains edge vector's dot products
            // For the calculations to be correct, we need them to be
            // oriented correctly
            using Point = Domain::MeshType::PointType;
            std::vector<Point> rv(this->edges[cell]); // Cell edge vectors

            const auto cellEntity =
                mesh.template getEntity<Domain::dCell>(cell);
            const auto cellCenter =
                TNL::Meshes::getEntityCenter(mesh, cellEntity);

            for (LocalIndex k = 0; k < this->edges[cell]; ++k) {
                const auto edge =
                    mesh.template getSubentityIndex<
                        Domain::dCell, Domain::dEdge
                    >(cell, k);

                const auto p1 = mesh.getPoint(
                    mesh.template getSubentityIndex<Domain::dEdge, 0>(
                        edge, 0
                    )
                );
                const auto p2 = mesh.getPoint(
                    mesh.template getSubentityIndex<Domain::dEdge, 0>(
                        edge, 1
                    )
                );

                const auto r = p2 - p1;

                const auto edgeEntity =
                    mesh.template getEntity<Domain::dEdge>(edge);
                const auto n = TNL::Meshes::getOutwardNormalVector(
                    mesh, edgeEntity, cellCenter
                );

                const auto crossZ = n[0] * r[1] - n[1] * r[0];
                rv[k] = (1 - 2 * (crossZ < 0)) * r;
                // rv[k] = (crossZ < 0) ? -r : r
            }

            for (LocalIndex i = 0; i < this->edges[cell]; ++i) {
                for (LocalIndex j = 0; j < this->edges[cell]; ++j) {
                    const auto index =
                        (cell * this->maxEdges + j) * this->maxEdges + i;
                    this->Binv.at(index) =
                        (rv[i], rv[j]) / this->measures[cell]
                        + 1.0 / this->l[cell] / 3.0;
                }
            }
        } // <-- for (...) fill Binv

        // Fill capacities
        *this->capacities <<
            [&prob, &mesh, this] (auto edge, auto& capacity) {
                // Row capacities for a system matrix
                // Each row contains as many nonzero elements as there are
                // edges in the cells adjacent to the corresponding edge
                capacity = 1;

                if (prob.dirichletMask[edge]) return;

                for (LocalIndex c = 0; c < localCells[edge]; ++c) {
                    const auto cell =
                        mesh.template getSuperentityIndex<
                            Domain::dEdge, Domain::dCell
                        >(edge, c);
                    capacity += this->edges[cell] - 1;
                }
            };

        // Create the linear system matrix
        this->M = std::make_shared<SparseMatrix>(edges, edges);
        this->M->setRowCapacities(*this->capacities);
        *this->M << 0;

        // Fill the linear system matrix
        mesh.template forAll<Domain::dEdge>(
            [&mesh, &prob, this] (auto edge) {
                this->M->addElement(
                    edge, edge, prob.dirichletMask[edge], 1
                );

                if (prob.dirichletMask[edge] && !prob.neumannMask[edge]) {
                    return;
                }

                for (LocalIndex c = 0; c < this->localCells[edge]; ++c) {
                    const auto cell =
                        mesh.template getSuperentityIndex<
                            Domain::dEdge, Domain::dCell
                        >(edge, c);

                    this->addMTerm(prob, cell, edge);
                }
            }
        );

        // Initialize solver/preconditioner
        this->solver =
            TNL::Solvers::getLinearSolver<SparseMatrix>(this->solverName);
        this->preconditioner =
            TNL::Solvers::getPreconditioner<SparseMatrix>(
                this->preconditionerName
            );

        preconditioner->update(this->M);
        solver->setMatrix(this->M);
        solver->setPreconditioner(this->preconditioner);

        this->isUpdated = true;
    } // <-- void cache(prob)

public:
    /// @brief Default constructor. Creates and binds layers
    MHFE(Problem& prob)
        : solution(
            prob.template addLayer<Real>(
                Domain::dCell, std::string{name} + " solution"
            )
          )
        , prevSolution(prob.template addLayer<Real>(Domain::dCell))
        , edgeSolution(prob.template addLayer<Real>(Domain::dEdge))
        , capacities(prob.template addLayer<GlobalIndex>(Domain::dEdge))
        , localCells(prob.template addLayer<LocalIndex>(Domain::dEdge))
        , edges(prob.template addLayer<LocalIndex>(Domain::dCell))
        , measures(prob.template addLayer<Real>(Domain::dCell))
        , lambda(prob.template addLayer<Real>(Domain::dCell))
        , alpha_i(prob.template addLayer<Real>(Domain::dCell))
        , alpha(prob.template addLayer<Real>(Domain::dCell))
        , beta(prob.template addLayer<Real>(Domain::dCell))
        , l(prob.template addLayer<Real>(Domain::dCell))
        , rhs(prob.template addLayer<Real>(Domain::dEdge))
    { this->cache(prob); }

    /// @brief Copy task
    MHFE(utils::combine::TaskCopy, const MHFE& other, Problem& prob)
        : solution(other.solution.copy(prob.getDomainForChange()))
        , prevSolution(other.prevSolution.copy(prob.getDomainForChange()))
        , edgeSolution(other.edgeSolution.copy(prob.getDomainForChange()))
        , capacities(other.capacities.copy(prob.getDomainForChange()))
        , localCells(other.localCells.copy(prob.getDomainForChange()))
        , edges(other.edges.copy(prob.getDomainForChange()))
        , measures(other.measures.copy(prob.getDomainForChange()))
        , lambda(other.lambda.copy(prob.getDomainForChange()))
        , alpha_i(other.alpha_i.copy(prob.getDomainForChange()))
        , alpha(other.alpha.copy(prob.getDomainForChange()))
        , beta(other.beta.copy(prob.getDomainForChange()))
        , l(other.l.copy(prob.getDomainForChange()))
        , rhs(other.rhs.copy(prob.getDomainForChange()))
    { this->cache(prob); }

    /// @brief Move task
    MHFE(utils::combine::TaskMove, MHFE&& other, Problem& prob)
    : MHFE(utils::combine::TaskCopy{}, other, prob)
    {}

    // Remove default move-copy operations
    MHFE(const MHFE&) = delete;
    MHFE& operator=(const MHFE&) = delete;
    MHFE(MHFE&&) = delete;
    MHFE& operator=(MHFE&&) = delete;

    /// @brief MHFE step
    void run(Problem& prob) {
        this->isUpdated = false;

        // Back up previous step's solution
        *this->prevSolution = *this->solution;

        using namespace noa::utils::tnl::op;
        // Update the RHS vector
        *this->rhs = 0;
        const auto& mesh = prob.getDomain().getMesh();
        mesh.template forAll<Domain::dEdge>(
            [&mesh, &prob, this] (auto edge) {
                this->rhs[edge] =
                    prob.neumannMask[edge] * prob.neumann[edge]
                    + prob.dirichletMask[edge] * prob.dirichlet[edge];

                if (prob.dirichletMask[edge] && !prob.neumannMask[edge]) {
                    return;
                }

                for (LocalIndex c = 0; c < this->localCells[edge]; ++c) {
                    const auto cell =
                        mesh.template getSuperentityIndex<
                            Domain::dEdge, Domain::dCell
                        >(edge, c);

                    if constexpr (lumping) {
                        this->rhs[edge] +=
                            prob.c[cell] * this->measures[cell]
                            * this->edgeSolution[edge] / 3.0
                            / prob.getTau();
                    } else {
                        this->rhs[edge] +=
                            prob.a[cell] * this->lambda[cell] *
                            this->solution[cell] / this->l[cell]
                            / this->beta[cell];
                    }
                }
            }
        );

        // Solve the linear system
        this->solver->solve(*this->rhs, *this->edgeSolution);

        // Update cell-wise solution
        *this->solution << [&mesh, &prob, this] (auto cell, auto& pv) {
            pv =
                this->prevSolution[cell] * this->lambda[cell]
                / this->beta[cell];

            for (LocalIndex lei = 0; lei < this->edges[cell]; ++lei) {
                const auto edge =
                    mesh.template getSubentityIndex<
                        Domain::dCell, Domain::dEdge
                    >(cell, lei);
                pv +=
                    prob.a[cell] * this->edgeSolution[edge]
                    / this->beta[cell] / this->l[cell];
            }
        };
    } // <-- void run(prob)

    /// @brief Problem update triggers cached values recalculation
    void onUpdated(const Problem& prob) { this->cache(prob); }

    [[nodiscard]] const SparseMatrix& getM() const { return *this->M; }

    [[nodiscard]] auto getLambda() const
    { return this->lambda->getConstView(); }

    [[nodiscard]] auto getBeta() const
    { return this->beta->getConstView(); }

    [[nodiscard]] auto getAlphaI() const
    { return this->alpha_i->getConstView(); }

    [[nodiscard]] auto getAlpha() const
    { return this->alpha->getConstView(); }

    [[nodiscard]] auto getEdges() const
    { return this->edges->getConstView(); }

    [[nodiscard]] auto getLocalCells() const
    { return this->localCells->getConstView(); }

    [[nodiscard]] auto getMeasures() const
    { return this->measures->getConstView(); }

    [[nodiscard]] auto getCapacities() const
    { return this->capacities->getConstView(); }

    [[nodiscard]] auto getMaxEdges() const { return this->maxEdges; }

    [[nodiscard]] const auto& getBinv() const
    { return this->Binv; }

    /// @brief Get cell-wise solution
    [[nodiscard]] auto getSolution() const
    { return this->solution->getConstView(); }

    /// @brief Get cell-wise solution from the previous step
    [[nodiscard]] auto getPreviousStepSolution() const
    { return this->prevSolution->getConstView(); }

    /// @brief Get edge-wise solution
    [[nodiscard]] auto getEdgeSolution() const
    { return this->edgeSolution->getConstView(); }

    /// @brief Solve the linear system with (L)MHFE matrix and given RHS
    void solve(
        Vector<Real>::ConstViewType rhs,
        Vector<Real>::ViewType out
    ) const {
        this->solver->solve(rhs, out);
    } // <-- void solve(rhs, out)
}; // <-- struct MHFE<Domain, lumping> (Triangle)

} // <-- namespace noa::cfd
