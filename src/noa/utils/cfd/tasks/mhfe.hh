/**
 * \file mhfe.hh
 * \brief Contains a task for solving a CFD problem with MHFE
 *
 * MHFE = Mixed Hybrid Finite Element method
 */

#pragma once

// Standard headers
#include <memory>

// NOA headers
#include <noa/utils/combine/combine.hh>
#include <noa/utils/domain/domain.hh>
#include <noa/3rdparty/tnl-noa/src/TNL/Containers/Vector.h>

// Local headers
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
 * \tparam MethodType type describing MHFE method (LMHFE or MHFE)
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
    /// \brief Domain type deduced from MethodType
    using DomainType = meta::GetSingleArg<MethodType>;
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

private:
    /// \brief Current simulation time
    RealType time{};
    /// \brief Simulation time step
    RealType tau{};

    /// \brief System sparse matrix type
    using SparseMatrixType = TNL::Matrices::SparseMatrix<RealType, DeviceType, GlobalIndexType>;

    /// \brief TNL Vector type
    template <typename DataType>
    using VectorType = TNL::Containers::Vector<DataType, DeviceType, GlobalIndexType>;

    /// \brief System matrix row capacities
    domain::LayerView<GlobalIndexType, DomainType> capacities;

    /// \brief MHFEM system matrix
    std::shared_ptr<SparseMatrixType> M;
    /// \brief System right-hand-side vector
    ProblemType::RealLayerView rhs;

    /// \brief Cached cell measures
    ProblemType::RealLayerView measure;

public:
    /// \brief Default constructor
    MHFE(ProblemType& problem) {
        this->time = RealType{};
        this->tau  = RealType{};

        problem.requestLayer(DomainType::dimCell, this->measure, RealType{});
        problem.requestLayer(DomainType::dimEdge, this->rhs,     RealType{});

        problem.requestLayer(DomainType::dimEdge, this->capacities, GlobalIndexType{});
    } // <-- MHFE()

    MHFE(const MHFE&) = delete;
    MHFE& operator=(const MHFE&) = delete;
    MHFE(MHFE&&) = default;
    MHFE& operator=(MHFE&&) = default;

    /// \brief Perform MHFE solution step
    void run() {
    } // <-- void MHFE::run()

private:
}; // <-- struct MHFE (Triangle)

} // <-- namespace noa::utils::cfd::tasks
