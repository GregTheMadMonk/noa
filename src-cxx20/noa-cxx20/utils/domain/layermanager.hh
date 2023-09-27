/*****************************************************************************
 *   Copyright (c) 2022, Roland Grinis, GrinisRIT ltd.                       *
 *   (roland.grinis@grinisrit.com)                                           *
 *   All rights reserved.                                                    *
 *   See the file COPYING for full copying permissions.                      *
 *                                                                           *
 *   This program is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation, either version 3 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the            *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 *****************************************************************************/
/**
 * Implemented by: Gregory Dushkin
 */

#pragma once

// STL headers
#include <cassert>
#include <variant>
#include <vector>
#include <map>

// TNL headers
#include <noa/3rdparty/tnl-noa/src/TNL/Containers/Vector.h>

// NOA headers
#include <noa/utils/exceptions.hh>

namespace noa::utils::domain {

// struct Layer
// A wrapper class to contain one mesh baked data layer
template <typename Device = TNL::Devices::Host, typename Index = std::size_t>
struct Layer {
        /* ----- PUBLIC TYPE ALIASES ----- */
        /// Types should be declared inside the `Layer` struct because they depend on layer
        /// template parameters
        template <typename DataType> using Vector = TNL::Containers::Vector<DataType, Device, Index>;

        /// TNL::Meshes::Readers::MeshReader stores data baked in mesh in an
        /// std::vector. We want to use TNL Vectors to possibly store layers
        /// on different devices, so we also need to deine our own std::variant
        /// for TNL Vectors instead of using TNL::Meshes::Readers::MeshReader::VariantVector
        using VariantVector = std::variant < Vector< std::int8_t >,
                                                Vector< std::uint8_t >,
                                                Vector< std::int16_t >,
                                                Vector< std::uint16_t >,
                                                Vector< std::int32_t >,
                                                Vector< std::uint32_t >,
                                                Vector< std::int64_t >,
                                                Vector< std::uint64_t >,
                                                Vector< float >,
                                                Vector< double > >;

        protected:
        /* ----- PROTECTED DATA MEMBERS ----- */
        VariantVector data;     // Layer data
        Index size;             // Layer size

        public:
        /* ----- PUBLIC DATA MEMBERS ----- */
        /// \brief Alternative layer name
        std::string alias = "";
        /// \brief Should this layer be saved (only a hint, all layers could still be saved)
        bool exportHint = false;

        /* ----- PUBLIC CONSTRUCTOR ----- */
        /// \brief Layer constructor. Calls \ref init()
        template <typename DataType>
        Layer(const Index& newSize, const DataType& value = DataType()) {
                init(newSize, value);
        }
        /* ----- PUBLIC METHODS ----- */
        /// \brief (Re-)Initialize layer data and fill it with values
        ///
        /// \param newSize - layer size
        /// \param value   - initializer value for all layer elements
        template <typename DataType>
        void init(const Index& newSize, const DataType& value = DataType()) {
                size = newSize;
                data = Vector<DataType>(size, value);
        }

        // Resize layer
        void setSize(const Index& newSize) {
                size = newSize;
                std::visit(
                    [&] (auto& v) {
                        v.setSize(size);
                    }, this->data
                );
        }

        // Get layer size
        const Index& getSize() const { return size; }

        // Visit the data
        template <typename Func>
        auto visit(Func&& f) { return std::visit(f, this->data); }

        // Visit the data (const)
        template <typename Func>
        auto visit(Func&& f) const { return std::visit(f, this->data); }

        // Set layer data from std::vector of same size
        template <typename DataType>
        void setFrom(const std::vector<DataType>& from) {
                assert(("Layer::setFrom(): Source size must be equal to layer size", size == from.size()));

                data = Vector<DataType>(size);
                std::get<Vector<DataType>>(data) = from;
        }

        // Get layer element
        template <typename DataType>
        DataType& operator[](const Index& index) {
                return std::get<Vector<DataType>>(data)[index];
        }

        template <typename DataType>
        const DataType& operator[](const Index& index) const {
                return std::get<Vector<DataType>>(data)[index];
        }

        // Get layer data vector
        template <typename DataType>
        Vector<DataType>& get() {
                return std::get<Vector<DataType>>(data);
        }

        template <typename DataType>
        const Vector<DataType>& get() const {
                return std::get<Vector<DataType>>(data);
        }

        // Write layer using a TNL mesh writer
        template <typename Writer>
        void writeCellData(Writer& writer, const std::string& fallback_name) const {
            std::visit(
                [&] (auto& v) {
                    writer.writeCellData(v, (alias.empty()) ? fallback_name : alias);
                }, this->data
            );
        }

        template <typename Writer>
        void writeDataArray(Writer& writer, const std::string& fallback_name) const {
            std::visit(
                [&] (auto& v) {
                        writer.writeDataArray(v, (alias.empty()) ? fallback_name : alias);
                }, this->data
            );
        }

        template <typename Writer>
        void writePointData(Writer& writer, const std::string& fallback_name) const {
            std::visit(
                [&] (auto& v) {
                    writer.writePointData(v, (alias.empty()) ? fallback_name : alias);
                }, this->data
            );
        }
}; // <-- struct Layer

// struct LayerManager
// A class to hold a multitude of `Layer`s of the same size
// (you're supposed to have one LayerManager per mesh dimension that you
// want to store data over)
template <typename Device = TNL::Devices::Host, typename Index = int>
struct LayerManager {
        /* ----- PUBLIC TYPE ALIASES ----- */
        /// \brief Layer type corresponding to this layer manager type
        using LayerType = Layer<Device, Index>;
        /// \brief Layer vector type
        template <typename DataType> using Vector = LayerType::template Vector<DataType>;

        /// \brief Device type
        using DeviceType = Device;
        /// \brief Index type
        using IndexType = Index;

        protected:
        /* ----- PROTECTED DATA MEMBERS ----- */
        Index size;
        /// Ordered map ensures that re-writing a file without change will give an identical per-byte copy
        std::map<std::size_t, Layer<Device, Index>> layers;

        public:
        /* ----- PUBLIC METHODS ----- */
        /// Sets size for all stored layers
        void setSize(const Index& newSize) {
                size = newSize;
                for (auto& layer : layers) layer.second.setSize(size);
        }
        /// Return the number of stored layers
        std::size_t count() const { return layers.size(); }

        /// Layer map start iterator
        auto begin() { return this->layers.begin(); }
        /// Layer map start iterator (const override)
        auto begin() const { return this->layers.cbegin(); }
        /// Layer map end iterator
        auto end() { return this->layers.end(); }
        /// Layer map end iterator (const override)
        auto end() const { return this->layers.cend(); }

        /// \brief Remove all layers
        void reset() {
            while (!layers.empty()) layers.erase(layers.begin());
        }

        /// \brief Remove all layers (deprecated usage). Use \ref reset()
        [[deprecated]] void clear() { this->reset(); }

        /// Add a layer storing a certain data type and returns its index
        template <typename DataType>
        LayerType& add(std::size_t alias, const DataType& value = DataType()) {
            layers.emplace(alias, LayerType(size, value));
            return layers.at(alias);
        }

        /// \brief Remove the layer at the alias
        void remove(std::size_t alias) {
            layers.erase(alias);
        }

        /// Get the first non-taken layer key
        std::size_t getNextLayerIndex() const {
            for (std::size_t idx = 0; idx < std::numeric_limits<std::size_t>::max(); ++idx) {
                if (!layers.contains(idx)) {
                    return idx;
                }
            }

            throw errors::FallthroughError{};
        } // <-- getNextLayerIndex()

        /// Return selected layer's data
        template <typename DataType>
        Vector<DataType>& get(std::size_t index) {
            return layers.at(index).template get<DataType>();
        }

        template <typename DataType>
        const Vector<DataType>& get(std::size_t index) const {
            return layers.at(index).template get<DataType>();
        }

        /// Return selected layer (`Layer` object instead of data)
        LayerType& getLayer(std::size_t index) {
            return layers.at(index);
        }

        const LayerType& getLayer(std::size_t index) const {
            return layers.at(index);
        }
}; // <-- struct LayerManager

} // <-- namespace noa::utils::domain
