// Copyright (c) 2004-2023 Tomáš Oberhuber et al.
//
// This file is part of TNL - Template Numerical Library (https://tnl-project.org/)
//
// SPDX-License-Identifier: MIT

#pragma once

#include <stdexcept>

/**
 * \brief Namespace for TNL exceptions.
 */
namespace noa::TNL::Exceptions {

struct CudaSupportMissing : public std::runtime_error
{
   CudaSupportMissing()
   : std::runtime_error( "CUDA support is missing, but the program called a function which needs it. "
                         "Please recompile the program with CUDA support." )
   {}
};

}  // namespace noa::TNL::Exceptions
