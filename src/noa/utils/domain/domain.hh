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
 * \file domain.hh
 * \brief \ref Domain wraps TNL Mesh and provedes a simple interface for storing data over it
 *
 * Implemented by: Gregory Dushkin
 */

#pragma once

// STL headers
#include <optional>

// TNL headers
#include <noa/3rdparty/tnl-noa/src/TNL/Algorithms/parallelFor.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Meshes/DefaultConfig.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Meshes/Mesh.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Meshes/MeshBuilder.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Meshes/TypeResolver/resolveMeshType.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Meshes/Writers/VTUWriter.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Meshes/Geometry/getEntityCenter.h>

// NOA headers
#include <noa/utils/common.hh>

// Local headers
#include "configtagpermissive.hh"
#include "domain_concepts.hh"
#include "layermanager.hh"
#include "layer_view_base.hh"

/// Namespace containing the domain-related code
namespace noa::utils::domain {

/// \brief Domain stores a TNL mesh and various data over its elements in one place
/// providing a friendly all-in-one-place interface for solvers.
template <typename CellTopology, typename Device = TNL::Devices::Host, typename Real = float, typename GlobalIndex = long int, typename LocalIndex = short int>
struct Domain {
        /* ----- PUBLIC TYPE ALIASES ----- */
        /// TNL Mesh config
        using MeshConfig        = TNL::Meshes::DefaultConfig<CellTopology, CellTopology::dimension, Real, GlobalIndex, LocalIndex>;
        /// \brief TNL Mesh type
        ///
        /// Doesn't use the \p Device template parameter.
        /// (TODO?) Look into CUDA Mesh implementation in TNL
        using MeshType          = TNL::Meshes::Mesh<MeshConfig>; // Meshes only seem to be implemented on Host (?)
        /// TNL Mesh writer type
        using MeshWriter        = TNL::Meshes::Writers::VTUWriter<MeshType>;
        /// LayerManager type
        using LayerManagerType  = LayerManager<Device, GlobalIndex>;

        /// Floating point numeric type
        using RealType          = Real;
        /// TNL mesh device type
        using DeviceType        = Device;
        /// Mesh global index type
        using GlobalIndexType   = GlobalIndex;
        /// Mesh local index type
        using LocalIndexType    = LocalIndex;

        /// Mesh point type
        using PointType         = typename MeshType::PointType;

        /// Get mesh dimensions
        [[deprecated]] static constexpr auto getMeshDimension() { return MeshType::getMeshDimension(); }

        /// \brief Mesh cell dimension
        static constexpr auto dimCell = MeshType::getMeshDimension();
        /// \brief Mesh edge dimension
        static constexpr auto dimEdge = dimCell - 1;

        protected:
        /* ----- PROTECTED DATA MEMBERS ----- */
        /// Domain mesh
        std::optional<MeshType> mesh = std::nullopt;
        /// Mesh data layers
        std::vector<LayerManagerType> layers;

        /// \brief LayerViewBase is our firend!
        friend class detail::LayerViewBase<Domain>;
        /// \brief Layer views bound to this domain
        std::vector<detail::LayerViewBase<Domain>*> layerViews;

        /* ----- PROTECTED METHODS ----- */
        /// Updates all layer sizes from mesh entities count
        template <int fromDimension = dimCell>
        void updateLayerSizes() {
                const auto size = isClean() ? 0 : mesh.value().template getEntitiesCount<fromDimension>();
                layers.at(fromDimension).setSize(size);
                if constexpr (fromDimension > 0) updateLayerSizes<fromDimension - 1>();
        }

        public:
        /* ----- PUBLIC METHODS ----- */
        /// Constructor
        Domain() {
                // If default-constructed, generate layers for each of the meshes dimensions
                layers = std::vector<LayerManagerType>(dimCell + 1);
        }

        /// \brief Copy-constructor
        ///
        /// Calls copy-assignment
        Domain(const Domain& other) {
            *this = other;
        } // <-- Domain(const Domain&)

        /// \brief Move-constructor
        ///
        /// Calls move-assignment
        Domain(Domain&& other) {
            *this = std::move(other);
        } // <-- Domain(Domain&&)

        /// \brief Copy-assignment
        ///
        /// Copying is implemented trivially except that the bound layer views
        /// aren't copied into new layer
        Domain& operator=(const Domain& other) {
            this->mesh = other.mesh;
            this->layers = other.layers;

            return *this;
        } // <-- operator=(const Domain&)

        /// \brief Move-assignment
        ///
        /// Implmented trivially except for the layer views. Layer views need to
        /// be unbound from the previous domain and re-bound into the new one
        Domain& operator=(Domain&& other) {
            this->mesh = std::move(other.mesh);
            this->layers = std::move(other.layers);

            while (!other.layerViews.empty()) {
                other.layerViews.at(0)->bindTo(*this);
            }

            return *this;
        } // <-- operator=(Domain&&)

        /// \brief Destructor
        ///
        /// Unbinds views
        ~Domain() {
            for (auto* view : layerViews) view->unbindFrom();
        }

        /// \brief Clear mesh data
        ///
        /// Resets both mesh and layer data
        void clear() {
                if (isClean()) return; // Nothing to clear

                // Reset the mesh
                mesh.reset();

                // Remove all the layers
                clearLayers();
        }

        /// Clear layer data
        void clearLayers() { for (auto& layer : layers) layer.clear(); }

        /// Check if the domain contains any layers
        [[nodiscard]] bool hasLayers() const { return this->layers.size(); }

        /// Check if the domain is empty. Equivalent to `!mesh.has_value()`
        [[nodiscard]] bool isClean() const { return !mesh.has_value(); }

        /// Check if all of the domain's bound layer views are valid
        [[nodiscard]] bool allViewsAreValid() const {
            for (const auto* view : this->layerViews) {
                if (!view->isValid()) return false;
            }
            return true;
        } // <-- bool allViewsAreValid()

        /// Get mesh as a constant reference
        const MeshType& getMesh() const { return mesh.value(); }

        /// Get center coordinates for entity with index
        template <int dimension>
        auto getEntityCenter(GlobalIndex idx) const {
                return TNL::Meshes::getEntityCenter(*this->mesh, this->mesh->template getEntity<dimension>(idx));
        }

        /// Get layers
        LayerManagerType& getLayers(const std::size_t& dimension) {
                return layers.at(dimension);
        }
        const LayerManagerType& getLayers(const std::size_t& dimension) const {
                return layers.at(dimension);
        }

        /// \brief Set domain mesh
        ///
        /// Updates all layers according to new mesh sizes
        void setMesh(const MeshType& newMesh) {
            mesh = newMesh;
            updateLayerSizes();

            for (auto* view : layerViews) view->bindTo(*this);
        }

        /// \brief Cell layers requested for loadFrom() function
        ///
        /// Contains pairs of ( layer_name -> layer_key )
        using LayersRequest = std::map<std::string, std::size_t>;

        private:
        /// \brief Layer loading helper
        ///
        /// \tparam DataType - required layer data type
        /// \tparam FromType - TNL's MeshReader::VariantVector, gets deduced automatically to avoid hardcoding
        ///
        /// \param from - VariantVector with data
        ///
        /// Handles loading a layer from a variant of `std::vector`'s when the
        /// layer data type is known
        template <typename DataType, typename FromType>
        void load_layer(std::size_t key, const FromType& from) {
                const auto& fromT = std::get<std::vector<DataType>>(from);
                auto& toT = this->getLayers(dimCell).template add<DataType>(key).template get<DataType>();

                for (std::size_t i = 0; i < fromT.size(); ++i)
                        toT[i] = fromT[i];
        }

        public:
        /// \brief Load Domain from a file
        /// \param filename - path to mesh file
        /// \param layersRequest - a list of requested cell layers
        ///
        /// Only supports loading of the cell layers (layers for cell dimension, \ref dimCell).
        /// Layers are loaded in order in which they were specified in \p layersRequest.
        void loadFrom(const Path& filename, const LayersRequest& layersRequest = {}) {
            if (!isClean())
                throw std::runtime_error("Mesh data is not empty, cannot load!");

            if (!std::filesystem::exists(filename))
                    throw std::runtime_error("Mesh file not found: " + filename.string() + "!");

            auto loader = [&] (auto& reader, auto&& loadedMesh) {
                    using LoadedTypeRef = decltype(loadedMesh);
                    using LoadedType = typename std::remove_reference<LoadedTypeRef>::type;

                    if constexpr (std::is_same_v<MeshType, LoadedType>) {
                        this->setMesh(loadedMesh);
                    } else throw std::runtime_error("Read mesh type differs from expected!");

                    // Load cell layers
                    for (auto& [ name, index ] : layersRequest) {
                            const auto data = reader.readCellData(name);

                            switch (data.index()) {
                                    case 0: /* int8_t */
                                            load_layer<std::int8_t>(index, data);
                                            break;
                                    case 1: /* uint8_t */
                                            load_layer<std::uint8_t>(index, data);
                                            break;
                                    case 2: /* int16_t */
                                            load_layer<std::int16_t>(index, data);
                                            break;
                                    case 3: /* uint16_t */
                                            load_layer<std::uint16_t>(index, data);
                                            break;
                                    case 4: /* int32_t */
                                            load_layer<std::int32_t>(index, data);
                                            break;
                                    case 5: /* uint32_t */
                                            load_layer<std::uint32_t>(index, data);
                                            break;
                                    case 6: /* int64_t */
                                            load_layer<std::int64_t>(index, data);
                                            break;
                                    case 7: /* uint64_t */
                                            load_layer<std::uint64_t>(index, data);
                                            break;
                                    case 8: /* float */
                                            load_layer<float>(index, data);
                                            break;
                                    case 9: /* double */
                                            load_layer<double>(index, data);
                                            break;
                            }

                            this->getLayers(dimCell).getLayer(index).alias = name;
                            this->getLayers(dimCell).getLayer(index).exportHint = true;
                    }

                    return true;
            };

            using ConfigTag = ConfigTagPermissive<CellTopology>;
            if (!TNL::Meshes::resolveAndLoadMesh<ConfigTag, TNL::Devices::Host>(loader, filename, "auto"))
                throw std::runtime_error("Could not load mesh (resolveAndLoadMesh returned `false`)!");
        }

        /// Write Domain to a file
        void write(const Path& filename) const {
                assert(("Mesh data is empty, nothing to save!", !isClean()));

                std::ofstream file(filename);
                if (!file.is_open())
                        throw std::runtime_error("Cannot open file " + filename.string() + "!");

                writeStream(file);
        }

        /// Write domain to an ostream
        void writeStream(std::ostream& stream) const {
                MeshWriter writer(stream);
                writer.template writeEntities<dimCell>(mesh.value());

                // Write layers
                for (int dim = 0; dim <= dimCell; ++dim)
                        for (const auto it : layers.at(dim)) {
                                const auto& layer = it.second;
                                if (!layer.exportHint) continue;
                                if (dim == dimCell)
                                        layer.writeCellData(writer, "cell_layer_" + std::to_string(it.first));
                                else if (dim == 0)
                                        layer.writePointData(writer, "point_layer_" + std::to_string(it.first));
                                else
                                        layer.writeDataArray(writer, "dim" + std::to_string(dim) + "_layer_" + std::to_string(it.first));
                        }
        }

        /* ----- DOMAIN MESH GENERATORS ----- */
        // Declare as friends since they're supposed to modify mesh
        friend void generate2DGrid(CDomainWithTopology<CellTopology> auto& domain,
                                std::size_t Nx,
                                std::size_t Ny,
                                float dx,
                                float dy,
                                float offsetX,
                                float offsetY);
}; // <-- struct Domain

/// \brief Specialization for Triangle topology
void generate2DGrid(
    CDomainWithTopology<TNL::Meshes::Topologies::Triangle> auto& domain,
    std::size_t Nx,
    std::size_t Ny,
    float dx,
    float dy,
    float offsetX = 0,
    float offsetY = 0
) {
        assert(("Mesh data is not empty, cannot create grid!", domain.isClean()));

        using CellTopology =    TNL::Meshes::Topologies::Triangle;
        using DomainType =      std::remove_reference_t<decltype(domain)>;
        using MeshType =        DomainType::MeshType;
        using GlobalIndexType = DomainType::GlobalIndexType;

        domain.mesh = MeshType();

        using Builder = TNL::Meshes::MeshBuilder<MeshType>;
        Builder builder;

        const auto elems = Nx * Ny * 2;
        const auto points = (Nx + 1) * (Ny + 1);
        builder.setEntitiesCount(points, elems);

        using PointType = typename MeshType::MeshTraitsType::PointType;
        const auto pointId = [&] (const int& ix, const int& iy) -> GlobalIndexType {
            return ix + (Nx + 1) * iy;
        };
        const auto fillPoints = [&] (const TNL::Containers::StaticArray<2, std::size_t>& pointIdx) {
            const auto& ix = pointIdx[0];
            const auto& iy = pointIdx[1];
            builder.setPoint(pointId(ix, iy), PointType(ix * dx + offsetX, iy * dy + offsetY));
        };
#ifdef HAVE_OPENMP
        using HostDevice = TNL::Devices::Sequential;
#else
        using HostDevice = TNL::Devices::Host;
#endif
        TNL::Algorithms::parallelFor<HostDevice>(
            TNL::Containers::StaticArray<2, std::size_t>{0,      0      },
            TNL::Containers::StaticArray<2, std::size_t>{Nx + 1, Ny + 1 },
            fillPoints
        );

        const auto fillElems = [&] (const TNL::Containers::StaticArray<3, std::size_t>& elemIdx) {
            const auto& ix = elemIdx[0];
            const auto& iy = elemIdx[1];
            const auto& u  = elemIdx[2];

            const auto cell = 2 * (ix + Nx * iy) + u;
            auto seed = builder.getCellSeed(cell);

            switch (u) {
                    case 1:
                            seed.setCornerId(0, pointId(ix, iy));
                            seed.setCornerId(1, pointId(ix, iy + 1));
                            seed.setCornerId(2, pointId(ix + 1, iy));
                            break;
                    case 0:
                            seed.setCornerId(0, pointId(ix + 1, iy + 1));
                            seed.setCornerId(1, pointId(ix + 1, iy));
                            seed.setCornerId(2, pointId(ix, iy + 1));
                            break;
            }
        };
        TNL::Algorithms::parallelFor<HostDevice>(
            TNL::Containers::StaticArray<3, std::size_t>{0,  0,  0},
            TNL::Containers::StaticArray<3, std::size_t>{Nx, Ny, 2},
            fillElems
        );

        builder.build(domain.mesh.value());

        domain.updateLayerSizes();
}

} // <-- namespace noa::utils::domain
