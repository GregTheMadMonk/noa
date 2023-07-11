// Copyright (c) 2004-2023 Tomáš Oberhuber et al.
//
// This file is part of TNL - Template Numerical Library (https://tnl-project.org/)
//
// SPDX-License-Identifier: MIT

#pragma once

namespace noa::TNL::Problems {

template< typename Real, typename Device, typename Index >
class Problem
{
public:
   using RealType = Real;
   using DeviceType = Device;
   using IndexType = Index;
};

}  // namespace noa::TNL::Problems
