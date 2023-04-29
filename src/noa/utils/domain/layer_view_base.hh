/**
 * \file layer_view_base.hh
 * \brief Base structure definition for LayerView
 */

#pragma once

namespace noa::utils::domain {

/// \brief Detail namespace
namespace detail {
/**
 * \brief Base type for LayerView
 *
 * Serves to allow dynamic storage of LayerView pointers
 * in Domain
 *
 * \tparam DomainType Domain type. Cannot apply CDomain constraint here
 *         to avoid "constraint depends on itself"
 */
template <typename DomainType>
class LayerViewBase {
protected:
    /// \brief Is the view valid (filled with valid data or garbage after reallocation)
    bool valid = false;

    /// \brief Get Domain views
    static auto& getDomainViews(DomainType& domain) { return domain.layerViews; }

public:
    // Ensure no meaningles assignment is performed by dereferencing the base class pointer
    LayerViewBase& operator=(const LayerViewBase&) = delete;
    LayerViewBase& operator=(LayerViewBase&&) = delete;

    /// \brief Virtual constructor ensures proper destruction of children
    virtual ~LayerViewBase() = default;

    /// \brief Check if the view is valid
    [[nodiscard]] bool isValid() const noexcept {
        return this->valid;
    }

    /// \brief Bind to domain
    ///
    /// See \ref LayerView::bindTo()
    virtual void bindTo(DomainType& domain) = 0;

    /// \brief Unbind from the current domain
    ///
    /// See \ref LayerView::unbindFrom()
    virtual void unbindFrom() = 0;
};

} // <-- namespace detail

} // <-- namespace noa::utils::domain
