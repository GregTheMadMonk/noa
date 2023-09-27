/**
 * @file layer_view.hh
 * @brief Domain layer view
 */
#pragma once

// Standard library
#include <cstdint>

// TNL headers
#include <noa/3rdparty/tnl-noa/src/TNL/Containers/Vector.hpp>

// NOA headers
#include <noa-cxx20/utils/meta.hh>

// Local headers
#include "domain.hh"

namespace noa::utils::domain {

template <typename DataType, meta::InstanceOf<Domain> DomainType>
class ConstLayerView;

/**
 * @brief Domain layer view class
 *
 * Stores a TNL Vector view to the stored layer's data as well as the
 * layer dimension and index.
 */
template <typename DataType, meta::InstanceOf<Domain> DomainType>
class LayerView {
    /// @brief Layer dimension
    std::size_t dimension;
    /// @brief Layer index
    std::size_t layerIndex;
    /// @brief Associated domain. Storing a pointer is safe as there is no
    ///        public access to it
    DomainType* domain;

    friend class ConstLayerView<DataType, DomainType>;

public:
    /// @brief Const view type
    using ConstView = ConstLayerView<DataType, DomainType>;

    /// @brief View constructor
    LayerView(
        std::size_t dimension, std::size_t layerIndex, DomainType& domain
    ) : dimension(dimension)
      , layerIndex(layerIndex)
      , domain(&domain)
    {}

    /// @brief Access the vector via `*`
    auto& operator*() {
        return this->domain->getLayers(this->dimension)
            .template get<DataType>(this->layerIndex);
    }
    /// @brief Access the vector via `*` (const)
    const auto& operator*() const {
        return this->domain->getLayers(this->dimension)
            .template get<DataType>(this->layerIndex);
    }

    /// @brief Access the vector via `->`
    auto* operator->() { return &this->operator*(); }
    /// @brief Access the vector via `->` (const)
    const auto* operator->() const { return &this->operator*(); }

    /// @brief Get the vector view
    auto view() { return (*this)->getView(); }
    /// @brief Get the const vector view
    auto view() const { return (*this)->getConstView(); }

    /// @brief Get layer element
    DataType& operator[](std::size_t idx)
    { return (*this)->operator[](idx); }
    /// @brief Get layer element (const)
    const DataType& operator[](std::size_t idx) const
    { return (*this)->operator[](idx); }

    /// @brief Get layer index
    auto index() const { return this->layerIndex; }

    /// @brief Return a view to the same layer on a different domain
    template <meta::InstanceOf<Domain> OtherDomain>
    LayerView<DataType, OtherDomain>
    copy(OtherDomain& otherDomain) const {
        return LayerView<DataType, OtherDomain>(
            this->dimension, this->layerIndex, otherDomain
        );
    } // <-- LayerView<DataType, OtherDomain> copy(otherDomain)
}; // <-- class LayerView

/// @brief Const domain layer view
template <typename DataType, meta::InstanceOf<Domain> DomainType>
class ConstLayerView {
    /// @brief Layer dimension
    std::size_t dimension;
    /// @brief Layer index
    std::size_t layerIndex;
    /// @brief Associated domain
    const DomainType* domain;

public:
    /// @brief View constructor
    ConstLayerView(
        std::size_t dimension,
        std::size_t layerIndex,
        const DomainType& domain
    ) : dimension(dimension)
      , layerIndex(layerIndex)
      , domain(&domain)
    {}

    /// @brief Construct from mutable view
    ConstLayerView(LayerView<DataType, DomainType> other)
    : dimension(other.dimension)
    , layerIndex(other.layerIndex)
    , domain(other.domain)
    {}

    /// @brief Access the vector via `*`
    const auto& operator*() const {
        return this->domain->getLayers(this->dimension)
            .template get<DataType>(this->layerIndex);
    } // <-- const auto& operator*() const

    /// @brief Access the vector via `->`
    auto* operator->() const { return &this->operator*(); }

    /// @brief Get the const vector view
    auto view() const { return (*this)->getConstView(); }

    /// @brief Get layer element
    const DataType& operator[](std::size_t idx) const
    { return (*this)->operator[](idx); }

    /// @brief Get layer index
    auto index() const { return this->layerIndex; }

    /// @brief Return a view to the same layer on a different domain
    template <meta::InstanceOf<Domain> OtherDomain>
    ConstLayerView<DataType, OtherDomain>
    copy(OtherDomain& otherDomain) const {
        return ConstLayerView<DataType, OtherDomain>(
            this->dimension, this->layerIndex, otherDomain
        );
    } // <-- LayerView<DataType, OtherDomain> copy(otherDomain)
}; // <-- class ConstLayerView

} // <-- namespace noa::utils::domain
