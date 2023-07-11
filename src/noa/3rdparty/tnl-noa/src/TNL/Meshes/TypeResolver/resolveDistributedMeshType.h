// Copyright (c) 2004-2023 Tomáš Oberhuber et al.
//
// This file is part of TNL - Template Numerical Library (https://tnl-project.org/)
//
// SPDX-License-Identifier: MIT

#pragma once

#include <noa/3rdparty/tnl-noa/src/TNL/Meshes/Mesh.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Meshes/Grid.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Meshes/DistributedMeshes/DistributedMesh.h>

namespace noa::TNL::Meshes {

template< typename ConfigTag, typename Device, typename Functor >
[[nodiscard]] bool
resolveDistributedMeshType( Functor&& functor, const std::string& fileName, const std::string& fileFormat = "auto" );

template< typename ConfigTag, typename Device, typename Functor >
[[nodiscard]] bool
resolveAndLoadDistributedMesh( Functor&& functor,
                               const std::string& fileName,
                               const std::string& fileFormat = "auto",
                               const MPI::Comm& communicator = MPI_COMM_WORLD );

template< typename Mesh >
[[nodiscard]] bool
loadDistributedMesh( DistributedMeshes::DistributedMesh< Mesh >& distributedMesh,
                     const std::string& fileName,
                     const std::string& fileFormat = "auto",
                     const MPI::Comm& communicator = MPI_COMM_WORLD );

}  // namespace noa::TNL::Meshes

#include <noa/3rdparty/tnl-noa/src/TNL/Meshes/TypeResolver/resolveDistributedMeshType.hpp>
