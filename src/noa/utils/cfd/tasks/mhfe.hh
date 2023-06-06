/**
 * \file mhfe.hh
 * \brief Contains a task for solving a CFD problem with MHFE
 *
 * MHFE = Mixed Hybrid Finite Element method
 */

#pragma once

// Standard headers
#include <memory>
#include <string>

// NOA headers
#include <noa/3rdparty/tnl-noa/src/TNL/Containers/Vector.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Solvers/LinearSolverTypeResolver.h>
#include <noa/utils/combine/combine.hh>
#include <noa/utils/domain/domain.hh>

// Local headers
#include "cfd_problem.hh"
#include "../scalar_function.hh"

// getEntityMeasure should be included after Mesh.h
#include <noa/3rdparty/tnl-noa/src/TNL/Meshes/Geometry/getEntityMeasure.h>

namespace noa::utils::cfd::tasks {

/**
 * \brief MHFE task base template
 *
 * \tparam DomainType Domain type
 * \tparam useLumping use lumped MHFE instead of regular MHFE
 *
 * Specialized for different domain topologies below
 */
template <domain::CDomain DomainType, bool useLumping>
struct MHFE; // Incomplete

/**
 * \brief MHFE specialization for triangular meshes
 *
 * TODO: MHFEM implementation could be generalized for all 2D cases
 */
template <domain::CDomain DomainType, bool useLumping>
requires domain::CDomainWithTopology<DomainType, TNL::Meshes::Topologies::Triangle>
struct MHFE<DomainType, useLumping> : public combine::MakeDynamic<MHFE<DomainType, useLumping>> {
    /// \brief CFD problem type
    using ProblemType = CFDProblem<DomainType>;

    /// \brief Domain real type
    using RealType = DomainType::RealType;
    /// \brief Domain device type
    using DeviceType = DomainType::DeviceType;
    /// \brief Domain global index type
    using GlobalIndexType = DomainType::GlobalIndexType;
    /// \brief Domain local index type
    using LocalIndexType = DomainType::LocalIndexType;

    /// \brief System sparse matrix type
    using SparseMatrixType = TNL::Matrices::SparseMatrix<RealType, DeviceType, GlobalIndexType>;

    /// \brief Simulation time step
    RealType tau{};

private:
    /// \brief Current simulation time
    RealType time{};

    /// \brief TNL Vector type
    template <typename DataType>
    using VectorType = TNL::Containers::Vector<DataType, DeviceType, GlobalIndexType>;

    /// \brief System matrix row capacities
    domain::LayerView<GlobalIndexType, DomainType> capacities;

    /// \brief Number of cells adjacent to an edge
    ProblemType::IntLayerView localCells;
    /// \brief Amount of edges per cell
    ProblemType::IntLayerView edges;
    /// \biref Max amount of edges per cell
    LocalIndexType maxEdges;

    /// \brief Solution on the previous step
    ProblemType::RealLayerView pPrev;

    /// \brief lambda cell parameter
    ProblemType::RealLayerView lambda;
    /// \brief alpha_i cell parameter
    ProblemType::RealLayerView alpha_i;
    /// \brief alpha cell parameter
    ProblemType::RealLayerView alpha;
    /// \brief beta cell parameter
    ProblemType::RealLayerView beta;
    /// \brief l cell parameter
    ProblemType::RealLayerView l;

    /// \brief MHFEM system matrix
    std::shared_ptr<SparseMatrixType> M;
    /// \brief System right-hand-side vector
    ProblemType::RealLayerView rhs;

    /// \brief B^{-1} matrix elements
    std::vector<RealType> Binv;

    /// \brief Cached cell measures
    ProblemType::RealLayerView measures;

    // TODO: Remove this and provide getters where needed?
    template <auto scalarWrtP, domain::CDomain GEDomainType>
    requires CScalarFuncWrtP<decltype(scalarWrtP), GEDomainType>
    friend struct GradEv;

public:
    /// \biref Preconditioner name
    std::string preconditionerName = "diagonal";
    /// \brief Solver name
    std::string solverName         = "gmres";

    /// \brief Default constructor
    explicit MHFE(ProblemType& problem) {
        this->time = RealType{};
        this->tau  = RealType{};

        problem.requestLayer(DomainType::dimEdge, this->localCells, int{});

        problem.requestLayer(DomainType::dimCell, this->edges,      int{});
        problem.requestLayer(DomainType::dimCell, this->measures,   RealType{});

        problem.requestLayer(DomainType::dimCell, this->pPrev,      RealType{});

        problem.requestLayer(DomainType::dimCell, this->lambda,     RealType{});
        problem.requestLayer(DomainType::dimCell, this->alpha_i,    RealType{});
        problem.requestLayer(DomainType::dimCell, this->alpha,      RealType{});
        problem.requestLayer(DomainType::dimCell, this->beta,       RealType{});
        problem.requestLayer(DomainType::dimCell, this->l,          RealType{});

        problem.requestLayer(DomainType::dimEdge, this->rhs,        RealType{});

        problem.requestLayer(DomainType::dimEdge, this->capacities, GlobalIndexType{});
    } // <-- MHFE()

    MHFE(const MHFE&) = delete;
    MHFE& operator=(const MHFE&) = delete;
    MHFE(MHFE&&) = default;
    MHFE& operator=(MHFE&&) = default;

    /// \brief Get simulation time
    RealType getTime() const { return time; }

    /// \brief Perform MHFE solution step
    void run(ProblemType& problem) {
        if (problem.needsUpdate()) this->cache(problem);

        // Copy previous step's solution
        *this->pPrev = *problem.solution;

        // Update RHS vector with respect to border conditions
        this->fillRhs(problem);

        // Construct solver and preconditioner
        auto preconditioner = TNL::Solvers::getPreconditioner<SparseMatrixType>(this->preconditionerName);
        auto solver         = TNL::Solvers::getLinearSolver  <SparseMatrixType>(this->solverName);

        preconditioner->update(this->M);
        solver->setMatrix(this->M);
        //solver->setPreconditioner(preconditioner);

        solver->solve(*this->rhs, *problem.edgeSolution);

        // Update cell-wise solution
        const auto& mesh = problem.getDomain().getMesh();
        problem.solution.fill([&mesh, &problem, this] (auto cell, auto& pv) {
            pv = this->pPrev[cell] * this->lambda[cell] / this->beta[cell];

            for (auto lei = this->edges[cell] - 1; lei >= 0; --lei) {
                const auto edge = mesh.template getSubentityIndex<DomainType::dimCell, DomainType::dimEdge>(cell, lei);
                pv += problem.a[cell] * problem.edgeSolution[edge] / this->beta[cell] / this->l[cell];
            }
        });

        this->time += tau;
    } // <-- void MHFE::run()

private:
    /// \brief Calculate parameters that don't need to be updated every step
    void cache(const ProblemType& problem) {
        const auto& mesh = problem.getDomain().getMesh();
        const auto cells = mesh.template getEntitiesCount<DomainType::dimCell>();
        const auto edges = mesh.template getEntitiesCount<DomainType::dimEdge>();

        // Cache cells per edge
        this->localCells.fill([&mesh] (auto edge, auto& edgeCells) {
            edgeCells = mesh.template getSuperentitiesCount<DomainType::dimEdge, DomainType::dimCell>(edge);
        });

        // Cache edges per cell
        this->edges.fill([&mesh] (auto cell, auto& cellEdges) {
            cellEdges = mesh.template getSubentitiesCount<DomainType::dimCell, DomainType::dimEdge>(cell);
        });

        // Fill cell measures
        this->measures.fill([&mesh] (auto cell, auto& measure) {
            measure = TNL::Meshes::getEntityMeasure(mesh, mesh.template getEntity<DomainType::dimCell>(cell));
        });

        // Fill cell parameters
        this->l.fill([&mesh, this] (auto cell, auto& val) {
            RealType sqSum = 0;

            for (auto edge = this->edges[cell] - 1; edge >= 0; --edge) {
                const auto gEdgeIdx = mesh.template getSubentityIndex<DomainType::dimCell, DomainType::dimEdge>(cell, edge);

                const auto p1 = mesh.getPoint(mesh.template getSubentityIndex<DomainType::dimEdge, 0>(edge, 0));
                const auto p2 = mesh.getPoint(mesh.template getSubentityIndex<DomainType::dimEdge, 0>(edge, 1));

                const auto r = p2 - p1;

                sqSum += (r, r);
            }

            val = sqSum / 48.0 / this->measures[cell];
        });
        this->lambda.fill([&problem, this] (auto cell, auto& val) {
            val = problem.c[cell] * this->measures[cell] / this->tau;
        });
        this->alpha_i.fill([this] (auto cell, auto& val) {
            val = 1.0 / this->l[cell];
        });
        this->alpha.fill([this] (auto cell, auto& val) {
            val = this->edges[cell] * this->alpha_i[cell];
        });
        this->beta.fill([&problem, this] (auto cell, auto& val) {
            val = this->lambda[cell] + problem.a[cell] * this->alpha[cell];
        });

        // Fill B^{-1}
        this->maxEdges = [this] {
            LocalIndexType ret = 0;

            this->edges->forAllElements([&ret] (auto, auto value) {
                if (value > ret) ret = value;
            });

            return ret;
        } ();

        this->Binv = std::vector<RealType>(cells * this->maxEdges * this->maxEdges, RealType{0});
        // Binv elements will be stored contiguosly for each cell
        // |elem|elem|elem|elem|elem|elem| ...
        // |      row     |      row     | ...
        // |                    Binv       ...
        for (auto cell = cells - 1; cell >= 0; --cell) {
            // B^{-1} contains edge vector's dot products
            // For the calculation to be correct, we need them to be oriented correctly
            using PointType = DomainType::MeshType::PointType;
            std::vector<PointType> rv(this->edges[cell]); // Cell edge vectors

            const auto cellEntity = mesh.template getEntity<DomainType::dimCell>(cell);
            const auto cellCenter = TNL::Meshes::getEntityCenter(mesh, cellEntity);

            for (LocalIndexType k = 0; k < this->edges[cell]; ++k) {
                const auto edge = mesh.template getSubentityIndex<DomainType::dimCell, DomainType::dimEdge>(cell, k);
                const auto p1 = mesh.getPoint(mesh.template getSubentityIndex<DomainType::dimEdge, 0>(edge, 0));
                const auto p2 = mesh.getPoint(mesh.template getSubentityIndex<DomainType::dimEdge, 0>(edge, 1));

                const auto r = p2 - p1;

                const auto edgeEntity = mesh.template getEntity<DomainType::dimEdge>(edge);
                const auto n = TNL::Meshes::getOutwardNormalVector(mesh, edgeEntity, cellCenter);

                const auto crossZ = n[0] * r[1] - n[1] * r[0];

                rv[k] = (1 - 2 * (crossZ < 0)) * r;
                // rv[k] = (crossZ < 0) ? -r : r;
            }

            for (auto i = this->edges[cell] - 1; i >= 0; --i) {
                for (auto j = this->edges[cell] - 1; j >= 0; --j) {
                    const auto index = (cell * this->maxEdges + j) * this->maxEdges + i;

                    this->Binv.at(index) = (rv.at(i), rv.at(j)) / this->measures[cell] + 1.0 / this->l[cell] / 3.0;
                }
            }
        }

        // Fill capacities
        this->capacities.fill([&problem, &mesh] (auto edge, auto& capacity) {
            // Row capacities for a system matrix
            // Each row contains as many nonzero elements as there are
            // edges in the cells adjacent to the corresponding edge
            capacity = 1;

            // Exceptions are rows corresponding to edges with only Dirichlet conditions
            // There only have one element
            if (problem.dirichletMask[edge] && !problem.neumannMask[edge]) {
                return;
            }

            const auto localCells = mesh.template getSuperentitiesCount<DomainType::dimEdge, DomainType::dimCell>(edge);
            for (auto cell = localCells - 1; cell >= 0; --cell) {
                const auto gCellIdx = mesh.template getSuperentityIndex<DomainType::dimEdge, DomainType::dimCell>(edge, cell);
                capacity += mesh.template getSubentitiesCount<DomainType::dimCell, DomainType::dimEdge>(gCellIdx) - 1;
            }
        });

        // Create M
        this->M = std::make_shared<SparseMatrixType>(edges, edges);
        this->M->setRowCapacities(*this->capacities);
        this->M->forAllElements([] (auto, auto, auto, auto& value) { value = 0; });

        // Fill the linear system matrix
        mesh.template forAll<DomainType::dimEdge>([&mesh, &problem, this] (auto edge) {
            this->M->addElement(edge, edge, problem.dirichletMask[edge], 1);

            if (problem.dirichletMask[edge] && !problem.neumannMask[edge]) return;

            const auto localCells = mesh.template getSuperentitiesCount<DomainType::dimEdge, DomainType::dimCell>(edge);

            for (LocalIndexType cell = 0; cell < localCells; ++cell) {
                const auto gCellIdx = mesh.template getSuperentityIndex<DomainType::dimEdge, DomainType::dimCell>(edge, cell);
                this->matrixTerm(problem, gCellIdx, edge);
            }
        });
    }

    /// \brief Modify matrix with the contribution from a specific edge
    ///
    /// Contributions should be applied for each cell the edge is adjacent to
    ///
    /// \param problem   CFDProblem task
    /// \param cell      Global cell index
    /// \param edge      Global edge index
    ///
    /// Internal constexpr logic handles both MHFE and LMHFE
    void matrixTerm(const ProblemType& problem, GlobalIndexType cell, GlobalIndexType edge) {
        const auto& mesh = problem.getDomain().getMesh();

        const auto edgeLocal = [&mesh, edge, cell, this] {
            for (auto lei = this->edges[cell] - 1; lei >= 0; --lei) {
                const auto gEdge = mesh.template getSubentityIndex<DomainType::dimCell, DomainType::dimEdge>(cell, lei);
                if (edge == gEdge) return lei;
            }

            throw utils::errors::FallthroughError{};
        } ();

        for (auto lei = this->edges[cell] - 1; lei >= 0; --lei) {
            const auto gEdge = mesh.template getSubentityIndex<DomainType::dimCell, DomainType::dimEdge>(cell, lei);

            // lei <-> edgeLocal should not matter since B^{-1} is symmetrical
            const auto BinvIndex = (cell * this->maxEdges + lei ) * this->maxEdges + edgeLocal;

            if constexpr (useLumping) {
                // LMHFE
                this->M->addElement(
                    edge, gEdge,
                    problem.a[cell] * (
                        this->Binv[BinvIndex]
                        -
                        this->alpha_i[cell] * this->alpha_i[cell] / this->alpha[cell]
                    ) + (lei == edgeLocal) * (problem.c[cell] * this->measures[cell] / 3.0 / this->tau),
                    1
                );
            } else {
                // MHFE
                this->M->addElement(
                    edge, gEdge,
                    problem.a[cell] * (
                        this->Binv[BinvIndex]
                        -
                        problem.a[cell] / this->l[cell] / this->l[cell] / this->beta[cell]
                    ),
                    1
                );
            }
        }
    } // <-- matrixTerm()

    /// \brief Fill system right-hand-side vector
    ///
    /// \param problem   CFDProblem task
    ///
    /// Internal constexpr logic handles both MHFE and LMHFE
    void fillRhs(const ProblemType& problem) {
        this->rhs.fill(0);

        const auto mesh = problem.getDomain().getMesh();
        mesh.template forAll<DomainType::dimEdge>([&mesh, &problem, this] (auto edge) {
            this->rhs[edge] = problem.neumannMask[edge] * problem.neumann[edge] + problem.dirichletMask[edge] * problem.dirichlet[edge];

            if (problem.dirichletMask[edge] && !problem.neumannMask[edge]) return;

            const auto localCells = mesh.template getSuperentitiesCount<DomainType::dimEdge, DomainType::dimCell>(edge);

            for (LocalIndexType lCell = 0; lCell < localCells; ++lCell) {
                const auto cell = mesh.template getSuperentityIndex<DomainType::dimEdge, DomainType::dimCell>(edge, lCell);
                if constexpr (useLumping) {
                    this->rhs[edge] += problem.c[cell] * this->measures[cell] * problem.edgeSolution[edge] / 3.0 / this->tau;
                } else {
                    this->rhs[edge] += problem.a[cell] * this->lambda[cell] * problem.solution[cell] / this->l[cell] / this->beta[cell];
                }
            }

        });
    } // <-- fillRhs()

}; // <-- struct MHFE (Triangle)

} // <-- namespace noa::utils::cfd::tasks
