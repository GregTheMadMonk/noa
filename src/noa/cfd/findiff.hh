/**
 * @file findiff.hh
 * @brief Finite difference method for CFD problems
 */
#pragma once

// Standard library
#include <functional>
#include <vector>

// NOA headers
#include <noa/utils/combine/combine.hh>
#include <noa/utils/common/meta.hh>
#include <noa/utils/domain/domain.hh>
#include <noa/utils/tnl/tnlx.hh>

// Local headers
#include "problem.hh"

namespace noa::cfd {

/**
 * @brief Finite difference scalar function sensitivity wrt `a` calculator
 *
 * Every time a computation is run, performs `Ncells` similar computations
 * with a set parameter variation
 */
template <utils::combine::Task Method>
requires requires (const Method& m) {
    { m.getSolution() } -> utils::tnl::LinearContainer;
    { [] (typename Method::Problem) {} }; // Check for `Problem` subtype
//  requires utils::combine::GetDeps<Method>::Contains<CFDProblem>;
} class FinDiff {
    /// @brief Static composer type for method
    using Other = utils::combine::StaticComposer<Method>;
    /// @brief Problem type
    using Problem = Method::Problem;
    /// @brief Domain type
    using Domain = utils::meta::At0<Problem>;

    using Real = Problem::Real;

    /// @brief Other computations for finite difference calculation
    std::vector<Other> others;

    /// @brief Coefficient variance
    Real da = 1e-3;

    /// @brief Result
    Problem::template LayerView<Real> result;

    /// @brief Update the task
    void update(const Problem& prob) {
        this->others.clear();

        const auto& mesh = prob.getDomain().getMesh();
        const auto cells =
            mesh.template getEntitiesCount<Domain::dCell>();

        using GlobalIndex = Problem::GlobalIndex;
        for (GlobalIndex cell = 0; cell < cells; ++cell) {
            this->others.emplace_back(
                [cell, &prob, this] (Problem& otherProb) {
                    otherProb.setMesh(prob.getDomain().getMesh());

                    *otherProb.a = *prob.a;
                    *otherProb.c = *prob.c;
                    *otherProb.dirichlet = *prob.dirichlet;
                    *otherProb.dirichletMask = *prob.dirichletMask;
                    *otherProb.neumann = *prob.neumann;
                    *otherProb.neumannMask = *prob.neumannMask;

                    otherProb.a[cell] += this->da;

                    otherProb.setTau(prob.getTau());
                    otherProb.setTime(prob.getTime());
                }
            );
        }
    } // <-- void update(prob)

    template <const auto& methodName>
    struct TaskName {
        static constexpr auto len(const char* str) {
            std::size_t ret = 0;

            while (str[ret++] != '\0');

            return ret;
        }
        static constexpr auto prefix =
            std::same_as<Real, double> ? "dbl_FinDiff_" : "FinDiff_";
        char value[len(prefix) + len(methodName) - 1] = {};

        constexpr TaskName() {
            const auto offset = len(prefix);
            for (std::size_t i = 0; i < offset; ++i) {
                value[i] = prefix[i];
            }
            for (std::size_t i = 0; i < len(methodName); ++i) {
                value[i + offset - 1] = methodName[i];
            }
        }
    };

    static constexpr TaskName<Method::name> nameInner{};

public:
    static constexpr auto name = nameInner.value;

    /**
     * @brief The scalar function. Accepts a const vector view to solution
     *        as its sole argument and returns a scalar value based on it.
     */
    std::function<
        Real(typename Problem::template Vector<Real>::ConstViewType)
    > scalarFunc;

    /// @brief Constructor
    FinDiff(Problem& prob)
    : scalarFunc([] (auto) { return 0; })
    , result(
        prob.template addLayer<Real>(
            Domain::dCell,
            std::string{"Finite difference for "}
            + utils::combine::taskName<Method>().data()
        )
      )
    { this->update(prob); }

    /// @brief Run all other computations and calculate the derivative
    void run(const Method& method) {
        const auto base = this->scalarFunc(method.getSolution());

        using namespace utils::tnl::op;
        *this->result << [base, this] (auto cell) {
            this->others[cell].run();
            const auto val = this->scalarFunc(
                this->others[cell].template get<Method>().getSolution()
            );

            return (val - base) / this->da;
        };
    } // <-- void run(method)

    /// @brief Problem is updated
    void onUpdated(const Problem& prob) { this->update(prob); }

    [[nodiscard]] Real getDA() const { return this->da; }
    void setDA(Real newDA) {
        const Real delta = newDA - this->da;
        using namespace utils::tnl::op;
        *this->result << [delta, this] (auto cell, auto) {
            this->others[cell].template get<Problem>().a[cell] += delta;
        };
        this->da = newDA;
    }
}; // <-- class FinDiff<Method>

} // <-- namespace cfd
