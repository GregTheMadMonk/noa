// Copyright (c) 2004-2023 Tomáš Oberhuber et al.
//
// This file is part of TNL - Template Numerical Library (https://tnl-project.org/)
//
// SPDX-License-Identifier: MIT

#pragma once

#include <noa/3rdparty/tnl-noa/src/TNL/Config/ParameterContainer.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Solvers/BuildConfigTags.h>

namespace noa::TNL::Solvers {

template<
   template< typename Real, typename Device, typename Index, typename MeshType, typename ConfigTag, typename SolverStarter >
   class ProblemSetter,
   typename ConfigTag >
class SolverInitiator
{
public:
   static bool
   run( const Config::ParameterContainer& parameters );
};

}  // namespace noa::TNL::Solvers

#include <noa/3rdparty/tnl-noa/src/TNL/Solvers/SolverInitiator.hpp>
