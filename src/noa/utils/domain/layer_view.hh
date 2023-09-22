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
#include <noa/utils/common/meta.hh>

// Local headers
#include "domain.hh"

namespace noa::utils::domain {

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
    std::size_t index;
    /// @brief Associated domain. Storing reference is safe as there is no
    ///        public access to it
    DomainType& domain;

public:
    /// @brief View constructor
    LayerView(
        std::size_t dimension, std::size_t index, DomainType& domain
    ) : dimension(dimension)
      , index(index)
      , domain(domain)
    {}

    /// @brief Access the vector via `*`
    auto& operator*() {
        return this->domain.getLayers(this->dimension)
            .template get<DataType>(this->index);
    }
    /// @brief Access the vector via `*` (const)
    const auto& operator*() const {
        return this->domain.getLayers(this->dimension)
            .template get<DataType>(this->index);
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
}; // <-- class LayerView

} // <-- namespace noa::utils::domain
