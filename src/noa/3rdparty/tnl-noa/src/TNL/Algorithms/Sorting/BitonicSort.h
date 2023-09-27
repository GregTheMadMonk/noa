// Copyright (c) 2004-2023 Tomáš Oberhuber et al.
//
// This file is part of TNL - Template Numerical Library (https://tnl-project.org/)
//
// SPDX-License-Identifier: MIT

#pragma once

#include <noa/3rdparty/tnl-noa/src/TNL/Algorithms/Sorting/detail/bitonicSort.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Exceptions/NotImplementedError.h>

namespace noa::TNL::Algorithms::Sorting {

struct BitonicSort
{
   template< typename Array >
   void static sort( Array& array )
   {
      bitonicSort( array );
   }

   template< typename Array, typename Compare >
   void static sort( Array& array, const Compare& compare )
   {
      bitonicSort( array, compare );
   }

   template< typename Device, typename Index, typename Compare, typename Swap >
   void static inplaceSort( const Index begin, const Index end, const Compare& compare, const Swap& swap )
   {
      if constexpr( std::is_same_v< Device, Devices::Cuda > )
         bitonicSort( begin, end, compare, swap );
      else
         throw Exceptions::NotImplementedError( "inplace bitonic sort is implemented only for CUDA" );
   }
};

}  // namespace noa::TNL::Algorithms::Sorting
