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
 * @file domain.hh
 * @brief @ref Domain wraps TNL Mesh and provedes a simple interface for storing data over it
 *
 * Implemented by: Gregory Dushkin
 */

#pragma once

// STL headers
#include <array>
#include <optional>

// TNL headers
#include <noa/3rdparty/tnl-noa/src/TNL/Algorithms/parallelFor.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Meshes/DefaultConfig.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Meshes/Mesh.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Meshes/MeshBuilder.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Meshes/TypeResolver/resolveMeshType.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Meshes/Writers/VTUWriter.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Meshes/Geometry/getEntityCenter.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Meshes/Geometry/getOutwardNormalVector.h>

// NOA headers
#include <noa/utils/common.hh> // For type aliases

// Local headers
#include "configtagpermissive.hh"
#include "layermanager.hh"

/// Namespace containing the domain-related code
namespace noa::utils::domain {

/**
 * @brief Domain stores a TNL mesh and various data over its elements in one place
 *        providing a friendly all-in-one-place interface for solvers.
 */
template <
    typename TCellTopology,
    typename TDeviceType = TNL::Devices::Host,
    typename TRealType = float,
    typename TGlobalIndexType = long int,
    typename TLocalIndexType = short int
> struct Domain {
    // Expose template parameters
    using CellTopology    = TCellTopology;
    using DeviceType      = TDeviceType;
    using RealType        = TRealType;
    using GlobalIndexType = TGlobalIndexType;
    using LocalIndexType  = TLocalIndexType;

    /// @brief TNL Mesh config
    using MeshConfig     = TNL::Meshes::DefaultConfig<CellTopology, CellTopology::dimension, RealType, GlobalIndexType, LocalIndexType>;
    /// @brief TNL Mesh type
    using MeshType       = TNL::Meshes::Mesh<MeshConfig>;
    /// @brief TNL VTU writer
    using MeshWriterType = TNL::Meshes::Writers::VTUWriter<MeshType>;
    /// @brief TNL Mesh point type
    using PointType      = MeshType::PointType;

    /// @brief Mesh cell dimension
    static constexpr auto dCell = MeshType::getMeshDimension();
    /// @brief Mesh edge dimension
    static constexpr auto dEdge = dCell - 1;

    /// @brief Layer manager type
    using LayerManagerType         = LayerManager<DeviceType, GlobalIndexType>;
    /// @brief Layer manager vector type
    template <typename DataType>
    using Vector = LayerManagerType::template Vector<DataType>;
    /// @brief Layer manager vector view type
    template <typename DataType>
    using VectorView = Vector<DataType>::ViewType;
    /// @brief Layer manager const vector ciew type
    template <typename DataType>
    using VectorConstView = Vector<DataType>::ConstViewType;

    /// @brief Point info struct
    class Point {
        /// @brief Point index
        GlobalIndexType idx;
        /// @brief Coordinate
        PointType coords;

    public:
        /// @brief Grab the point info
        Point(GlobalIndexType idx, const MeshType& mesh)
        : idx(idx)
        , coords(mesh.getPoint(idx))
        {}

        const auto& index() const { return this->idx; }
        const auto& point() const { return this->coords; }
    }; // <-- class Point

    /// @brief Edge info struct
    class Edge {
        /// @brief Edge index
        GlobalIndexType idx;
        /// @brief Point coordinates
        std::vector<PointType> pts;
        /// @brief Edge center
        PointType c;
        /// @brief Edge normal
        PointType n;

    public:
        Edge(GlobalIndexType idx, const MeshType& mesh)
        : idx(idx) {
            const auto pointsCount =
                mesh.template getSubentitiesCount<dEdge, 0>(idx);

            this->pts.reserve(pointsCount);
            for (LocalIndexType pi = 0; pi < pointsCount; ++pi) {
                const auto point =
                    mesh.template getSubentityIndex<dEdge, 0>(idx, pi);
                this->pts.push_back(mesh.getPoint(point));
            }

            const auto edgeEntity =
                mesh.template getEntity<dEdge>(idx);

            this->c = TNL::Meshes::getEntityCenter(mesh, edgeEntity);

            this->n = TNL::Meshes::getOutwardNormalVector(
                mesh, edgeEntity, this->c
            );
        } // <-- Edge(idx, mesh)

        /// @brief Get edge points
        const auto& points() const { return this->pts; }

        /// @brief Get edge center
        const PointType& center() const { return this->c; }

        /// @brief Get edge normal
        const PointType& normal() const { return this->n; }
    }; // <-- class Edge

private:
    /// @brief Domain mesh
    std::optional<MeshType> mesh = std::nullopt;
    /// @brief Mesh data layers
    std::vector<LayerManagerType> layers{};

    /// @brief Updates all layers' sizes from mesh entities count
    template <int fromDimension = dCell>
    void updateLayerSizes() {
        const auto size = this->isClean() ? 0 : this->mesh->template getEntitiesCount<fromDimension>();
        this->layers.at(fromDimension).setSize(size);
        if constexpr (fromDimension > 0) this->updateLayerSizes<fromDimension - 1>();
    }

public:
    /// @brief Default constructor
    Domain() {
        // Generate layer managers for each one of the mesh dimensions
        this->layers = std::vector<LayerManagerType>(dCell + 1);
    } // <-- Domain()

    /// @brief Copy-constructor
    Domain(const Domain& other) = default;
    /// @brief Move-constructor
    Domain(Domain&& other) = default;

    /// @brief Copy-assignment
    Domain& operator=(const Domain& other) = default;
    /// @brief Move-assignment
    Domain& operator=(Domain&&) = default;

    /// @brief Destructor
    ~Domain() = default;

    /**
     * @brief Reset the domain
     *
     * Clears the mesh and layer data
     */
    void reset() {
        // If there is no mesh, data layers also
        // are not initialized
        if (!this->mesh.has_value()) return;

        this->mesh = std::nullopt;
        this->resetLayers();
    }

    /// @brief Reset layer data only
    void resetLayers() {
        for (auto& layer : this->layers) layer.reset();
    }

    /// @brief Check if the domain contains a mesh. Equivalent to `!mesh.has_value()`
    [[nodiscard]] bool isClean() const { return !this->mesh.has_value(); }

    /// @brief Check if the domain has any layers
    [[nodiscard]] bool hasLayers() const { return !this->layers.empty(); }

    /**
     * @brief Get a const reference to a stored mesh
     *
     * @throw std::bad_optional_access if there is no mesh stored as a result
     *        of `std::optional::value()` call
     */
    const MeshType& getMesh() const { return this->mesh.value(); }

    /**
     * @brief Get center coordinates for an entity with index
     *
     * @throw std::bad_optional_access if there is no mesh stored as a result
     *        of `std::optional::value()` call
     */
    template <int tDimension>
    auto getEntityCenter(GlobalIndexType index) const {
        return TNL::Meshes::getEntityCenter(
            this->mesh.value(),
            this->mesh->template getEntity<tDimension>(index)
        );
    } // <-- getEntityCenter(index)

    /// @brief Get a layer manager for dimension
    LayerManagerType& getLayers(const std::size_t& dimension) {
        return this->layers.at(dimension);
    } // <-- getLayers(dimension)
    const LayerManagerType& getLayers(const std::size_t& dimension) const {
        return this->layers.at(dimension);
    } // <-- getLayers(dimension) const

    /**
     * @brief Set domain mesh
     *
     * Sets the @ref mesh optional and updates all layers' sizes
     * accordigly
     */
    /// @brief Set domain mesh (move)
    template <typename MeshType_>
    requires std::same_as<std::remove_cvref_t<MeshType_>, MeshType>
    void setMesh(MeshType_&& newMesh) {
        this->mesh = std::forward<MeshType_>(newMesh);
        this->updateLayerSizes();
    } // <-- setMesh(newMesh) (move)

    /// @brief Fetch and get the full point info
    Point getPoint(GlobalIndexType idx) const
    { return Point{ idx, this->mesh.value() }; }

    /// @brief Fetch and get the full edge info
    Edge getEdge(GlobalIndexType idx) const
    { return Edge{ idx, this->mesh.value() }; }

    /// @brief Write domain to a file
    void write(const Path& filename) const {
        if (this->isClean()) {
            throw std::runtime_error("Domain contains no mesh data, nothing to save!");
        }

        std::ofstream(filename) << *this;
    } // <-- write(filename) const

    /**
     * @brief Write domain contents to an output stream
     *
     * @throw std::bad_optional_access if there is no mesh stored as a result
     *        of `std::optional::value()` call
     */
    friend std::ostream& operator<<(std::ostream& stream, const Domain& self) {
        // Write the mesh itself
        MeshWriterType writer(stream);
        writer.template writeEntities<dCell>(self.mesh.value());

        // Write mesh layers
        for (int dim = 0; dim <= dCell; ++dim) {
            for (const auto& [ index, layer ] : self.layers.at(dim)) {
                if (!layer.exportHint) continue;

                switch (dim) {
                case dCell:
                    layer.writeCellData(writer, "cell_layer_" + std::to_string(index));
                    break;
                case 0:
                    layer.writePointData(writer, "point_layer_" + std::to_string(index));
                    break;
                default:
                    layer.writeDataArray(writer, "dim" + std::to_string(dim) + "_layer_" + std::to_string(index));
                    break;
                }
            }
        }

        return stream;
    } // <-- operator<<(stream)

    /**
     * @brief Load the domain from a VTU file specified by filename
     *
     * @param filename   - VTU file name
     * @param cellLayers - a map of index->name for cell layers to load
     *
     * Currently, only cell layers loading is supported.
     * TODO: Load all available layers
     */
    void loadFrom(const Path& filename, const std::unordered_map<std::string, std::size_t>& cellLayers) {
        if (this->mesh.has_value()) {
            throw std::runtime_error("Can't load the domain: mesh data is not empty.");
        }

        const auto loader = [this, &cellLayers] (auto& reader, auto&& loadedMesh) {
            using LoadedTypeRef = decltype(loadedMesh);
            using LoadedType = std::remove_reference_t<LoadedTypeRef>;

            if constexpr (std::same_as<LoadedType, MeshType>) {
                this->setMesh(loadedMesh);
            } else {
                throw std::runtime_error("Incorrect mesh type (wrong topology?)");
            }

            // Load cell layers
            for (const auto& [ name, index ] : cellLayers) {
                const auto data = reader.readCellData(name);

                std::visit(
                    [this, index] <typename FromType> (const std::vector<FromType>& fromVector) {
                        auto& toT = this->getLayers(dCell).template add<FromType>(index).template get<FromType>();

                        for (std::size_t i = 0; i < fromVector.size(); ++i) toT[i] = fromVector[i];
                    }, data
                );

                this->getLayers(dCell).getLayer(index).alias = name;
                this->getLayers(dCell).getLayer(index).exportHint = true;
            }

            return true;
        };

        using ConfigTag = ConfigTagPermissive<CellTopology>;
        if (!TNL::Meshes::resolveAndLoadMesh<ConfigTag, TNL::Devices::Host>(loader, filename, "auto")) {
            throw std::runtime_error("Could not load mesh (resolveAndLoadMesh returned `false`)!");
        }
    } // <-- loadFrom(filename)

    /**
     * @brief Generate the grid mesh for this domain
     *
     * @param elements    - number of elements along all axes
     * @param elementSize - element size along all axes
     * @param offset      - domain origin point offset
     */
    void generateGrid(
        std::array<GlobalIndexType, dCell> elements,
        std::array<RealType,        dCell> elementSize,
        std::array<RealType,        dCell> offset = {}
    ) requires std::same_as<CellTopology, TNL::Meshes::Topologies::Triangle> {
        using BuilderType     = TNL::Meshes::MeshBuilder<MeshType>;

        const auto& [ Nx, Ny ] = elements;
        const auto& [ dx, dy ] = elementSize;
        const auto& [ ox, oy ] = offset;

        BuilderType builder;

        const auto elems = Nx * Ny * 2;
        const auto points = (Nx + 1) * (Ny + 1);
        builder.setEntitiesCount(points, elems);

        // Convert point grid position into index
        const auto pointId = [Nx] (GlobalIndexType ix, GlobalIndexType iy) {
            return ix + (Nx + 1) * iy;
        };

        // Fill points
        TNL::Algorithms::parallelFor<TNL::Devices::Host>(
            TNL::Containers::StaticArray<2, GlobalIndexType>{0,      0      },
            TNL::Containers::StaticArray<2, GlobalIndexType>{Nx + 1, Ny + 1 },
            [dx, dy, ox, oy, &builder, &pointId] (const TNL::Containers::StaticArray<2, GlobalIndexType>& pointIdx) {
                const auto& [ ix, iy ] = pointIdx;
                builder.setPoint(pointId(ix, iy), PointType(ix * dx + ox, iy * dy + oy));
            }
        );

        // Fill elements
        TNL::Algorithms::parallelFor<TNL::Devices::Host>(
            TNL::Containers::StaticArray<3, GlobalIndexType>{ 0,  0,  0 },
            TNL::Containers::StaticArray<3, GlobalIndexType>{ Nx, Ny, 2 },
            [Nx, &builder, &pointId] (const TNL::Containers::StaticArray<3, GlobalIndexType>& elemIdx) {
                const auto& [ ix, iy, u ] = elemIdx;

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
                default:
                    throw std::runtime_error("Invalid value of parameter u");
                }
            }
        );

        MeshType genMesh{};
        builder.build(genMesh);
        this->setMesh(std::move(genMesh));
    } // <-- void generate2DGrid(elements, elementSize, offset) | Triangle
}; // <-- struct Domain

} // <-- namespace noa::utils::domain
