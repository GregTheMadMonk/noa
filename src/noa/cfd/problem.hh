/**
 * @file problem.hh
 * @brief Definition of the initial task for CFD problems
 */
#pragma once

// NOA headers
#include <noa/utils/common/meta.hh>
#include <noa/utils/domain/domain.hh>
#include <noa/utils/domain/layer_view.hh>

namespace noa::cfd {

/**
 * @brief An error that is thrown when a solver ecounters a problem with
 *        invalid setup (no mesh, incomplete boundary conditions, etc)
 */
struct InvalidProblem : public std::runtime_error {
    InvalidProblem(std::string_view msg) : runtime_error{msg.data()} {}
}; // <-- struct InvalidProblem

/**
 * @brief Initial task for all CFD problems
 *
 * Initializes and holds the domain with initial conditions, medium
 * properties and basic solution layers, as well as providing basic info
 * to the CFD solver (such as simulation time and time step).
 */
template <utils::meta::InstanceOf<utils::domain::Domain> Domain>
class CFDProblem {
    /// @brief Computation domain
    Domain domain;

public:
    using Real = Domain::RealType;

    template <typename DataType>
    using LayerView = utils::domain::LayerView<DataType, Domain>;

    LayerView<Real> solution;
    LayerView<Real> a;
    LayerView<Real> c;

    LayerView<Real> dirichlet;
    LayerView<int>  dirichletMask;
    LayerView<Real> neumann;
    LayerView<int>  neumannMask;
    LayerView<Real> edgeSolution;

private:
    /// @brief Simulation time step
    Real tau;
    /// @brief Current simulation time
    Real time;
    /// @brief A flag to signal all dependant tasks about an update
    bool isUpdated = false;

    /// @brief Validate problem boundary conditions and correctness
    void validate() const {
        // Check if the domain has a mesh
        if (this->domain.isClean()) throw InvalidProblem{"Empty domain"};

        bool hasBoundary = true;
        this->domain.getMesh().template forBoundary<Domain::dEdge>(
            [this, &hasBoundary] (auto edge) {
                hasBoundary &= this->dirichletMask[edge] != 0
                                || this->neumannMask[edge] != 0;
            }
        );

        if (!hasBoundary) {
            throw InvalidProblem{"Incomplete boundary conditions"};
        }
    } // <-- void validate() const

public:
    /**
     * @brief Add a layer to the domain and return a view to it
     *
     * By default, the layer isn't exported and doesn't have an alias.
     * Settings \par alias not equal to `std::nullopt` results in the
     * created layer having an alias equal to `alias.value()` and
     * `exportHint = true`. Layer contents are default-initialized
     */
    template <typename DataType>
    [[nodiscard]] LayerView<DataType> addLayer(
        std::size_t dimension,
        std::optional<std::string> alias = std::nullopt
    ) {
        auto& manager = this->domain.getLayers(dimension);
        const auto idx = manager.getNextLayerIndex();

        auto& layer = manager.template add<DataType>(idx, DataType{});
        if (alias.has_value()) {
            layer.alias = *alias;
            layer.exportHint = true;
        }

        return LayerView<DataType>(dimension, idx, this->domain);
    } // <-- LayerView<DataType> addLayer(dimension, alias)

    /**
     * @brief Default constructor
     *
     * Populates the domain with layers. When the domain mesh is set,
     * they will persist and get resized accordingly
     */
    CFDProblem()
        : solution(addLayer<Real>(Domain::dCell, "Computed solution"))
        , a(addLayer<Real>(Domain::dCell)) 
        , c(addLayer<Real>(Domain::dCell))
        , dirichlet(addLayer<Real>(Domain::dEdge))
        , dirichletMask(addLayer<int>(Domain::dEdge))
        , neumann(addLayer<Real>(Domain::dEdge))
        , neumannMask(addLayer<int>(Domain::dEdge))
        , edgeSolution(addLayer<Real>(Domain::dEdge))
        , tau{0.005}
        , time{}
        , isUpdated{true}
    {}

    // TODO: TaskCopy/TaskMove

    // Remove default move-copy operations
    CFDProblem(const CFDProblem&) = delete;
    CFDProblem& operator=(const CFDProblem&) = delete;
    CFDProblem(CFDProblem&&) = delete;
    CFDProblem& operator=(CFDProblem&&) = delete;

    /**
     * @brief All CFDProblem does is validity checks on boundary
     *        conditions and resetting of the \ref isUpdated flag
     */
    void run() {
        this->isUpdated = false;
        this->validate();

        this->time += this->tau;
    } // <-- void run()

    /// @brief Signal children tasks that an update has occurred
    [[nodiscard]] bool updated() const noexcept { return this->isUpdated; }

    /// @brief Get the problem's domain
    [[nodiscard]] const Domain& getDomain() const { return this->domain; }

    /// @brief Set the domain's mesh. Triggers an updated state
    void setMesh(const typename Domain::MeshType& mesh) {
        this->domain.setMesh(mesh);
        this->isUpdated = true;
    } // <-- void setMesh(mesh)

    /// @brief Get time step value
    [[nodiscard]] Real getTau() const { return this->tau; }
    /// @brief Set time step value. Triggers an updated state
    void setTau(Real tau) { this->tau = tau; this->isUpdated = true; }
    /// @brief Get current simulation time
    [[nodiscard]] Real getTime() const { return this->time; }
}; // <-- struct CFDProblem<Domain>

} // <-- namespace noa::cfd
