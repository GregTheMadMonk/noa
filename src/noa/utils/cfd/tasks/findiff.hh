/**
 * \file findiff.hh
 * \brief Finite difference calculation of sensitivity
 */

#pragma once

#include <noa/utils/combine/combine.hh>

#include "../scalar_function.hh"
#include "mhfe.hh"

namespace noa::utils::cfd::tasks {

/// \brief Finite difference sensitivity calculation method
template <auto scalarF, typename BaseMethod, typename DomainType> // TODO remove DomainType parameter
requires CScalarFunc<decltype(scalarF), DomainType>
struct FinDiff : public combine::MakeDynamic<FinDiff<scalarF, BaseMethod, DomainType>> {
    using OtherComputation = combine::StaticComputation<BaseMethod>;
    using ProblemType      = BaseMethod::ProblemType;
    using RealType         = BaseMethod::RealType;

    /// \brief Finite difference step
    ///
    /// This is awful and should not be a public member but eh
    /// I'm in a hurry here TODO
    RealType da = 1e-3;

private:
    /// \brief Finite difference calculator runs several other computation of method
    ///        with slightly different parameters
    std::vector<OtherComputation> others;

    /// \brief Sensitivity calculation result
    typename ProblemType::RealLayerView result;

public:
    FinDiff(ProblemType& problem) {
        auto& resultLayer = problem.requestLayer(DomainType::dimCell, this->result, RealType{});
        resultLayer.alias = "Finite difference sensitivity";
        resultLayer.exportHint = true;

        problem.update();
    } // <-- FinDiff()

    void run(const ProblemType& problem, const BaseMethod& base) {
        if (problem.needsUpdate()) this->update(problem, base);

        const auto val = scalarF(*problem.solution) / this->da;
        for (std::size_t cell = 0; cell < this->others.size(); ++cell) {
            auto& comp = this->others[cell];

            if (cell == 0) comp.template get<ProblemType>().getDomain().write("temp3/" + std::to_string(base.getTime()) + ".vtu");

            this->result[cell] = scalarF(*comp.template get<ProblemType>().solution) / this->da;
            this->result[cell] -= val;

            comp.run();
        }
    } // <-- void run()

    /// \brief Get a const view to calculation result
    auto getResult() const { return this->result->getConstView(); }

private:
    /// \brief Update the setup
    void update(const ProblemType& problem, const BaseMethod& base) {
        const auto& mesh = problem.getDomain().getMesh();
        const auto cells = mesh.template getEntitiesCount<DomainType::dimCell>();

        this->others = std::vector<OtherComputation>(cells);

        for (std::size_t compIdx = 0; compIdx < this->others.size(); ++compIdx) {
            auto& otherProblem = this->others[compIdx].template get<ProblemType>();

            otherProblem.setMesh(mesh);
            otherProblem.solution.fill(problem.solution);
            otherProblem.a.fill(
                [&problem, compIdx, this] (auto idx, auto& v) {
                    v = problem.a[idx] + (compIdx == idx) * this->da;
                }
            );
            otherProblem.c.fill(problem.c);

            otherProblem.dirichlet.fill(problem.dirichlet);
            otherProblem.dirichletMask.fill(problem.dirichletMask);
            otherProblem.neumann.fill(problem.neumann);
            otherProblem.neumannMask.fill(problem.neumannMask);

            otherProblem.edgeSolution.fill(problem.edgeSolution);

            auto& otherBase = this->others[compIdx].template get<BaseMethod>();
            otherBase.tau = base.tau;
        }
    } // <-- void update()
}; // <-- struct FinDiff

} // <-- namespace noa::utils::cfd::tasks
