// Copyright (c) 2004-2023 Tomáš Oberhuber et al.
//
// This file is part of TNL - Template Numerical Library (https://tnl-project.org/)
//
// SPDX-License-Identifier: MIT

#pragma once

#include <noa/3rdparty/tnl-noa/src/TNL/Meshes/TypeResolver/BuildConfigTags.h>

namespace noa::TNL::Meshes {

template< typename ConfigTag, typename Device >
class GridTypeResolver
{
public:
   template< typename Reader, typename Functor >
   [[nodiscard]] static bool
   run( Reader& reader, Functor&& functor );

protected:
   template< typename Reader, typename Functor >
   struct detail
   {
      [[nodiscard]] static bool
      resolveGridDimension( Reader& reader, Functor&& functor );

      // NOTE: We could disable the grids only by the GridTag, but doing the
      //       resolution for all subtypes is more flexible and also pretty
      //       good optimization of compilation times.

      template< int MeshDimension >
      [[nodiscard]] static bool
      resolveReal( Reader& reader, Functor&& functor );

      template< int MeshDimension, typename Real >
      [[nodiscard]] static bool
      resolveIndex( Reader& reader, Functor&& functor );

      template< int MeshDimension, typename Real, typename Index >
      [[nodiscard]] static bool
      resolveGridType( Reader& reader, Functor&& functor );

      template< typename GridType >
      [[nodiscard]] static bool
      resolveTerminate( Reader& reader, Functor&& functor );
   };
};

}  // namespace noa::TNL::Meshes

#include <noa/3rdparty/tnl-noa/src/TNL/Meshes/TypeResolver/GridTypeResolver.hpp>
