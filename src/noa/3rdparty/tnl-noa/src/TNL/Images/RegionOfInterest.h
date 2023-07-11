// Copyright (c) 2004-2023 Tomáš Oberhuber et al.
//
// This file is part of TNL - Template Numerical Library (https://tnl-project.org/)
//
// SPDX-License-Identifier: MIT

#pragma once

#include <noa/3rdparty/tnl-noa/src/TNL/Config/ParameterContainer.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Images/Image.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Meshes/Grid.h>

namespace noa::TNL::Images {

template< typename Index = int >
class RegionOfInterest
{
public:
   RegionOfInterest();

   bool
   setup( const Config::ParameterContainer& parameters, const Image< Index >* image );

   [[nodiscard]] bool
   check( const Image< Index >* image ) const;

   [[nodiscard]] Index
   getTop() const;

   [[nodiscard]] Index
   getBottom() const;

   [[nodiscard]] Index
   getLeft() const;

   [[nodiscard]] Index
   getRight() const;

   [[nodiscard]] Index
   getWidth() const;

   [[nodiscard]] Index
   getHeight() const;

   template< typename Grid >
   bool
   setGrid( Grid& grid, bool verbose = false );

   [[nodiscard]] bool
   isIn( Index row, Index column ) const;

protected:
   Index top, bottom, left, right;
};

}  // namespace noa::TNL::Images

#include <noa/3rdparty/tnl-noa/src/TNL/Images/RegionOfInterest_impl.h>
