/**
 * \file layer_view.hh
 * \brief Domain layer view
 */

#pragma once

#include <cassert>
#include <concepts>

#include "domain.hh"
#include "layer_view_base.hh"

namespace noa::utils::domain {

/**
 * \brief Layer view struct
 *
 * Layer view inherits from TNL's vector view and introduces
 * one additional read-only members: layer index and dimension. It is needed
 * to store vector views to layer data and to be able to re-attach
 * them to different domains on moving/copying
 *
 * \tparam DataType   layer data type
 * \tparam DomainType host domain type
 */
template <typename DataType, CDomain DomainType>
class LayerView : public detail::LayerViewBase<DomainType>
{
    /// \brief Vector view type
    using ViewType = DomainType::LayerManagerType::template Vector<DataType>::ViewType;
    /// \brief LayerViewBase type
    using Base = detail::LayerViewBase<DomainType>;

    /// \brief Contained view
    ViewType view;

    /// \brief Current domain pointer
    DomainType* domain = nullptr;
    /// \brief Domain layer index
    std::size_t index;
    /// \brief Domain layer dimension
    std::size_t dimension;

public:
    /// \brief Default-constructor
    LayerView() {}

    /// \brief Construct the view from domain layer
    ///
    /// \param manager        domain layer manager
    /// \param layerDimension domain layer dimension
    /// \param layerIndex     domain layer index
    LayerView(DomainType& domain, std::size_t layerDimension, std::size_t layerIndex) :
        index(layerIndex),
        dimension(layerDimension)
    {
        this->bindTo(domain);
    }

    /// \brief Copy-constructor
    LayerView(const LayerView& other) {
        this->copyFrom(other);
    }

    /// \brief Move-constructor
    LayerView(LayerView&& other) {
        this->moveFrom(std::move(other));
    }

    /// \brief Copy-assignment
    LayerView& operator=(const LayerView& other) {
        this->copyFrom(other);
        return *this;
    }

    /// \brief Move-assignment
    LayerView& operator=(LayerView&& other) {
        this->moveFrom(std::move(other));
        return *this;
    }

    /// \brief Destructor
    ///
    /// Unbinds the view
    ~LayerView() override {
        this->unbindFrom();
    }

    /// \brief Dimension getter
    std::size_t getDimension() const { return this->dimension; }
    /// \brief Index getter
    std::size_t getIndex() const { return this->index; }

    /// \brief Access the stored view
    ViewType& operator*() { return this->view; }
    /// \brief Access the stored view
    const ViewType& operator*() const { return this->view; }

    /// \brief Access the stroed view member
    ViewType* operator->() { return &this->view; }
    /// \brief Access the stroed view member
    const ViewType* operator->() const { return &this->view; }

    /// \brief Bind to domain
    ///
    /// This will set \ref valid to `false`. This is done because
    /// when the domain is changed, even though the memory this view
    /// points to is available, there might still be garbage in there.
    ///
    /// Validation is performed via \ref fill()
    void bindTo(DomainType& domain) override {
        if (&domain != this->domain) {
            this->unbindFrom(); // This automatically sets \ref valid to `false`
            this->domain = &domain;
            auto& views = Base::getDomainViews(*this->domain);
            assert(std::find(views.begin(), views.end(), this) == views.end());
            views.push_back(this);
        }

        this->view.bind(this->domain->getLayers(this->dimension).template get<DataType>(this->index));
    } // <-- bindTo()

    /// \brief Unbind from Domain
    void unbindFrom() override {
        if (this->domain == nullptr) return;

        auto& views = Base::getDomainViews(*this->domain);
        auto it = std::find(views.begin(), views.end(), this);
        views.erase(it);
        assert(std::find(views.begin(), views.end(), this) == views.end());

        this->domain = nullptr;
        this->valid = false;
    } // <-- unbindFrom()

    /// \brief Fill the view
    ///
    /// Essentially calls TNL View's forAll. Sets \ref valid to `true`
    template <std::invocable<typename ViewType::IndexType, DataType&> Func>
    void fill(Func f) {
        this->view.forAllElements(f);
        this->valid = true;
    } // <-- fill()

    /// \brief Fill the view to be exactly like another view
    ///
    /// Sets \ref valid to `true`
    void fill(const LayerView& other) {
        this->view.forAllElements([&other] (auto idx, auto& v) { v = other[idx]; });
        this->valid = true;
    } // <-- fill()

    /// \brief Fill the view with a constant value
    ///
    /// Sets \ref valid to `true`
    void fill(DataType value) {
        this->view.forAllElements([value] (auto, auto& v) { v = value; });
        this->valid = true;
    } // <-- fill()

    /// \brief Set the view \ref valid flag to `true` but don't mutate it
    ///
    /// Should be used in case the view was already filled by an ouside code
    void fill() {
        this->valid = true;
    } // <-- fill()

    /// \brief Element getter
    DataType& operator[](typename ViewType::IndexType index) {
        return this->view[index];
    } // <-- operator[]()

    /// \brief Element getter (const)
    const DataType& operator[](typename ViewType::IndexType index) const {
        return this->view[index];
    } // <-- operator[]()
private:
    void copyFrom(const LayerView& other) {
        this->unbindFrom();

        this->index = other.index;
        this->dimension = other.dimension;

        if (other.domain != nullptr) {
            this->bindTo(*other.domain);
            this->valid = other.valid;
        }
    } // <-- copyFrom()

    void moveFrom(LayerView&& other) {
        this->unbindFrom();

        this->index = other.index;
        this->dimension = other.dimension;

        if (other.domain != nullptr) {
            this->bindTo(*other.domain);
            this->valid = other.valid;
            other.unbindFrom();
        }
    } // <-- moveFrom()
}; // <-- class LayerView

} // <-- namespace noa::utils::domain
