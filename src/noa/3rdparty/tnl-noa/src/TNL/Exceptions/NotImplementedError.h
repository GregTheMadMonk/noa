// Copyright (c) 2004-2023 Tomáš Oberhuber et al.
//
// This file is part of TNL - Template Numerical Library (https://tnl-project.org/)
//
// SPDX-License-Identifier: MIT

#pragma once

#include <stdexcept>

namespace noa::TNL::Exceptions {

struct NotImplementedError : public std::runtime_error
{
   NotImplementedError( const std::string& msg = "Something is not implemented." ) : std::runtime_error( msg ) {}
};

}  // namespace noa::TNL::Exceptions
