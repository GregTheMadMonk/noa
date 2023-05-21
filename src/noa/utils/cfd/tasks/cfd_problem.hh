/**
 * \file cfd_problem.hh
 * \brief Definition of the initial task for CFD problems
 */

#pragma once

// NOA headers
#include <noa/utils/combine/combine.hh>
#include <noa/utils/domain/domain.hh>
#include <noa/utils/domain/layer_view.hh>

// Local headers
#include "../exceptions.hh"

namespace noa::utils::cfd::tasks {

/**
 * \brief Initial task for all CFD problems
 *
 * Holds the domain with initial conditions/solution layers.
 * Serves as a base for all other CFD computations.
 */
template <domain::CDomain DomainType>
struct CFDProblem : public combine::MakeDynamic<CFDProblem<DomainType>> {
private:
    /// \brief Computation domain
    DomainType domain;

public:
    /// \brief Real type
    using RealType = DomainType::RealType;

    using RealLayerView = domain::LayerView<RealType, DomainType>;
    using IntLayerView =  domain::LayerView<int, DomainType>;

    /// \brief Solution layer view
    RealLayerView solution;
    /// \brief `a` coefficient layer view
    RealLayerView a;
    /// \brief `c` coefficient layer view
    RealLayerView c;

    /// \brief Dirichlet layer index
    RealLayerView dirichlet;
    /// \brief Diriclet conditions mask layer index
    IntLayerView  dirichletMask;
    /// \brief Neumann layer index
    RealLayerView neumann;
    /// \brief Neumann conditions mask layer index
    IntLayerView  neumannMask;
    /// \brief Edge solutions layer index
    RealLayerView edgeSolution;

    /// \brief Re-calculate all cached values
    ///
    /// Is set to `2` by \ref update(), and checked by `needsUpdate()`.
    /// Signs that a re-caclulation of cached values is required to be
    /// performed by all tasks depending on it
    int queueUpdate = 0;

public:
    /// \brief Default constructor
    ///
    /// Populates the domain with layers.
    /// When the domain mesh is set, layers persist and get resized
    /// accordingly
    CFDProblem() {
        auto solutionLayer = this->requestLayer(DomainType::dimCell, this->solution, RealType{});
        solutionLayer.alias = "Computed Solution";
        solutionLayer.exportHint = true;

        this->requestLayer(DomainType::dimCell, this->a, RealType{});
        this->requestLayer(DomainType::dimCell, this->c, RealType{});

        this->requestLayer(DomainType::dimEdge, this->dirichlet,     RealType{});
        this->requestLayer(DomainType::dimEdge, this->dirichletMask, int{});
        this->requestLayer(DomainType::dimEdge, this->neumann,       RealType{});
        this->requestLayer(DomainType::dimEdge, this->neumannMask,   int{});
        this->requestLayer(DomainType::dimEdge, this->edgeSolution,  RealType{});

        // First step needs to calculate all of the cache-able parameters
        this->update();
    }

    CFDProblem(const CFDProblem&) = delete;
    CFDProblem(CFDProblem&&) = default;
    CFDProblem& operator=(const CFDProblem&) = delete;
    CFDProblem& operator=(CFDProblem&&) = default;

    /// \brief Each time CFDProblem is used, it performs a validity check
    void run() {
        // Don't update on the next step
        if (this->queueUpdate > 0) --this->queueUpdate;

        if (!this->valid()) throw errors::InvalidSetup{};
    } // <-- void CFDProblem::run()

    /// \brief Set domain mesh
    /// 
    /// Automatically resizes all of initialized layers.
    ///
    /// \param mesh new mesh
    void setMesh(const typename DomainType::MeshType& mesh) {
        this->domain.setMesh(mesh);
    }

    /// \brief Request a new layer from a domain
    ///
    /// New layer gets added to layer re-binding index
    ///
    /// \param dimension layer dimension
    /// \param view      view to bind to the layer
    /// \param value     default value for layer elements
    template <typename DataType>
    auto& requestLayer(std::size_t dimension, domain::LayerView<DataType, DomainType>& view, DataType value) {
        auto& manager = this->domain.getLayers(dimension);
        const auto index = manager.getNextLayerIndex();

        auto& ret = manager.template add<DataType>(index, value); 

        view = domain::LayerView<DataType, DomainType>(this->domain, dimension, index);

        return ret;
    }

    /// \brief Rebind a view to this problem's domain
    template <typename DataType>
    void bindView(domain::LayerView<DataType, DomainType>& view) {
        view.bindTo(*this->domain);
    }
    
    /// \brief Get problem domain
    [[nodiscard]] const DomainType& getDomain() const { return this->domain; }

    /// \brief Sign to all dependant tasks that an update of all cached values is needed
    ///
    /// Sets \ref queueUpdate to `2`. This is needed to ensure that all dependant tasks
    /// still recieve update signal after a \ref run() is performed once
    void update() { this->queueUpdate = 2; }

    /// \brief Check if an update to all cached values is needed
    [[nodiscard]] bool needsUpdate() const { return this->queueUpdate > 0; }

private:
    /// \brief Validate CFD problem formulation
    [[nodiscard]] bool valid() const noexcept {
        // Check the domain is clean
        if (this->domain.isClean()) {
            std::cerr << "The this->domain is empty!" << std::endl;
            return false;
        }

        // We do not check `this->domain->allViewsAreValid()` here because
        // children tasks could have initialized and bound their own views
        // and might not have had a chance to refill them yet. Also, we do not
        // require for all of the views to be valid: solution is automatically
        // initialized with zero if invalid
        #ifdef NOA_CFD_PROBLEM_CHECK_VIEW
        #error "NOA_CFD_PROBLEM_CHECK_VIEW is already defined. Are you serious?!"
        #endif
        #define NOA_CFD_PROBLEM_CHECK_VIEW(view) \
        if (!this->view.isValid()) {\
            std::cerr << #view " layer view is invalid!" << std::endl;\
            return false;\
        }

        NOA_CFD_PROBLEM_CHECK_VIEW(solution);
        NOA_CFD_PROBLEM_CHECK_VIEW(a);
        NOA_CFD_PROBLEM_CHECK_VIEW(c);
        NOA_CFD_PROBLEM_CHECK_VIEW(dirichlet);
        NOA_CFD_PROBLEM_CHECK_VIEW(dirichletMask);
        NOA_CFD_PROBLEM_CHECK_VIEW(neumann);
        NOA_CFD_PROBLEM_CHECK_VIEW(neumannMask);
        NOA_CFD_PROBLEM_CHECK_VIEW(edgeSolution);
        #undef NOA_CFD_PROBLEM_CHECK_VIEW

        // Check that all boundaries have at least one boundary condition associated with them
        bool hasBoundary = true;
        this->domain.getMesh().template forBoundary<DomainType::dimEdge>(
            [this, &hasBoundary] (auto edge) {
                hasBoundary &= this->dirichletMask[edge] != 0 || this->neumannMask[edge] != 0;
            }
        );

        if (!hasBoundary) {
            std::cerr << "Domain boundary conditions missing on one or more boundary edges!";
            return false;
        }

        return true;
    } // <-- bool CFDProblem::valid()
}; // <-- struct CFDProblem

} // <-- namespace noa::utils::cfd::tasks
