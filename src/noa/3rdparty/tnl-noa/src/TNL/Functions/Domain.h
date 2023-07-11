// Copyright (c) 2004-2023 Tomáš Oberhuber et al.
//
// This file is part of TNL - Template Numerical Library (https://tnl-project.org/)
//
// SPDX-License-Identifier: MIT

#pragma once

namespace noa::TNL::Functions {

enum DomainType
{
   NonspaceDomain,
   SpaceDomain,
   MeshDomain,
   MeshInteriorDomain,
   MeshBoundaryDomain
};

template< int Dimension, DomainType DomainType_ = SpaceDomain >
class Domain
{
public:
   using DeviceType = void;

   [[nodiscard]] static constexpr int
   getDomainDimension()
   {
      return Dimension;
   }

   [[nodiscard]] static constexpr DomainType
   getDomainType()
   {
      return DomainType_;
   }
};

}  // namespace noa::TNL::Functions
