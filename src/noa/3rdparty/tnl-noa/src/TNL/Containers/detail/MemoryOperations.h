// Copyright (c) 2004-2023 Tomáš Oberhuber et al.
//
// This file is part of TNL - Template Numerical Library (https://tnl-project.org/)
//
// SPDX-License-Identifier: MIT

#pragma once

#include <noa/3rdparty/tnl-noa/src/TNL/Devices/Sequential.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Devices/Host.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Devices/Cuda.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Cuda/CudaCallable.h>

namespace noa::TNL::Containers::detail {

template< typename DestinationDevice >
struct MemoryOperations;

template<>
struct MemoryOperations< Devices::Sequential >
{
   template< typename Element, typename Index >
   __cuda_callable__
   static void
   construct( Element* data, Index size );

   // note that args are passed by reference to the constructor, not via
   // std::forward since move-semantics does not apply for the construction of
   // multiple elements
   template< typename Element, typename Index, typename... Args >
   __cuda_callable__
   static void
   construct( Element* data, Index size, const Args&... args );

   template< typename Element, typename Index >
   __cuda_callable__
   static void
   destruct( Element* data, Index size );

   template< typename Element >
   __cuda_callable__
   static void
   setElement( Element* data, const Element& value );

   template< typename Element >
   __cuda_callable__
   static Element
   getElement( const Element* data );
};

template<>
struct MemoryOperations< Devices::Host >
{
   template< typename Element, typename Index >
   static void
   construct( Element* data, Index size );

   // note that args are passed by reference to the constructor, not via
   // std::forward since move-semantics does not apply for the construction of
   // multiple elements
   template< typename Element, typename Index, typename... Args >
   static void
   construct( Element* data, Index size, const Args&... args );

   template< typename Element, typename Index >
   static void
   destruct( Element* data, Index size );

   // this is __cuda_callable__ only to silence nvcc warnings
   TNL_NVCC_HD_WARNING_DISABLE
   template< typename Element >
   __cuda_callable__
   static void
   setElement( Element* data, const Element& value );

   // this is __cuda_callable__ only to silence nvcc warnings
   TNL_NVCC_HD_WARNING_DISABLE
   template< typename Element >
   __cuda_callable__
   static Element
   getElement( const Element* data );
};

template<>
struct MemoryOperations< Devices::Cuda >
{
   template< typename Element, typename Index >
   static void
   construct( Element* data, Index size );

   // note that args are passed by value to the constructor, not via
   // std::forward or even by reference, since move-semantics does not apply for
   // the construction of multiple elements and pass-by-reference cannot be used
   // with CUDA kernels
   template< typename Element, typename Index, typename... Args >
   static void
   construct( Element* data, Index size, const Args&... args );

   template< typename Element, typename Index >
   static void
   destruct( Element* data, Index size );

   template< typename Element >
   __cuda_callable__
   static void
   setElement( Element* data, const Element& value );

   template< typename Element >
   __cuda_callable__
   static Element
   getElement( const Element* data );
};

}  // namespace noa::TNL::Containers::detail

#include "MemoryOperationsSequential.hpp"
#include "MemoryOperationsHost.hpp"
#include "MemoryOperationsCuda.hpp"
