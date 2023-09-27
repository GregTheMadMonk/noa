// Copyright (c) 2004-2023 Tomáš Oberhuber et al.
//
// This file is part of TNL - Template Numerical Library (https://tnl-project.org/)
//
// SPDX-License-Identifier: MIT

#pragma once

#include <algorithm>    // std::copy
#include <memory>       // std::unique_ptr
#include <stdexcept>    // std::length_error
#include <type_traits>  // std::remove_cv_t

#include <noa/3rdparty/tnl-noa/src/TNL/Assert.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Algorithms/parallelFor.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Exceptions/CudaSupportMissing.h>

#include "Copy.h"

namespace noa::TNL::Algorithms::detail {

template< typename DestinationElement, typename SourceElement, typename Index >
__cuda_callable__
void
Copy< Devices::Sequential >::copy( DestinationElement* destination, const SourceElement* source, Index size )
{
   if( size == 0 )
      return;
   TNL_ASSERT_TRUE( destination, "Attempted to copy data to a nullptr." );
   TNL_ASSERT_TRUE( source, "Attempted to copy data from a nullptr." );

   for( Index i = 0; i < size; i++ )
      destination[ i ] = source[ i ];
}

template< typename DestinationElement, typename Index, typename SourceIterator >
void
Copy< Devices::Sequential >::copy( DestinationElement* destination,
                                   Index destinationSize,
                                   SourceIterator begin,
                                   SourceIterator end )
{
   Index i = 0;
   while( i < destinationSize && begin != end )
      destination[ i++ ] = *begin++;
   if( begin != end )
      throw std::length_error( "Source iterator is larger than the destination array." );
}

template< typename DestinationElement, typename SourceElement, typename Index >
void
Copy< Devices::Host >::copy( DestinationElement* destination, const SourceElement* source, Index size )
{
   if( size == 0 )
      return;
   TNL_ASSERT_TRUE( destination, "Attempted to copy data to a nullptr." );
   TNL_ASSERT_TRUE( source, "Attempted to copy data from a nullptr." );

   // our ParallelFor version is faster than std::copy iff we use more than 1 thread
   if( Devices::Host::isOMPEnabled() && Devices::Host::getMaxThreadsCount() > 1 ) {
      auto kernel = [ destination, source ]( Index i )
      {
         destination[ i ] = source[ i ];
      };
      parallelFor< Devices::Host >( 0, size, kernel );
   }
   else {
      // std::copy usually uses std::memcpy for TriviallyCopyable types
      std::copy( source, source + size, destination );
   }
}

template< typename DestinationElement, typename Index, typename SourceIterator >
void
Copy< Devices::Host >::copy( DestinationElement* destination, Index destinationSize, SourceIterator begin, SourceIterator end )
{
   Copy< Devices::Sequential >::copy( destination, destinationSize, begin, end );
}

template< typename DestinationElement, typename SourceElement, typename Index >
void
Copy< Devices::Cuda >::copy( DestinationElement* destination, const SourceElement* source, Index size )
{
   if( size == 0 )
      return;
   TNL_ASSERT_TRUE( destination, "Attempted to copy data to a nullptr." );
   TNL_ASSERT_TRUE( source, "Attempted to copy data from a nullptr." );

   // our ParallelFor kernel is faster than cudaMemcpy
   auto kernel = [ destination, source ] __cuda_callable__( Index i )
   {
      destination[ i ] = source[ i ];
   };
   parallelFor< Devices::Cuda >( 0, size, kernel );
}

template< typename DestinationElement, typename Index, typename SourceIterator >
void
Copy< Devices::Cuda >::copy( DestinationElement* destination, Index destinationSize, SourceIterator begin, SourceIterator end )
{
   using BaseType = typename std::remove_cv_t< DestinationElement >;
   const int buffer_size = noa::TNL::min( Cuda::getTransferBufferSize() / sizeof( BaseType ), destinationSize );
   std::unique_ptr< BaseType[] > buffer{ new BaseType[ buffer_size ] };
   Index copiedElements = 0;
   while( copiedElements < destinationSize && begin != end ) {
      Index i = 0;
      while( i < buffer_size && begin != end )
         buffer[ i++ ] = *begin++;
      Copy< Devices::Cuda, Devices::Sequential >::copy( &destination[ copiedElements ], buffer.get(), i );
      copiedElements += i;
   }
   if( begin != end )
      throw std::length_error( "Source iterator is larger than the destination array." );
}

template< typename DeviceType >
template< typename DestinationElement, typename SourceElement, typename Index >
void
Copy< DeviceType, Devices::Cuda >::copy( DestinationElement* destination, const SourceElement* source, Index size )
{
   if( size == 0 )
      return;
   TNL_ASSERT_TRUE( destination, "Attempted to copy data to a nullptr." );
   TNL_ASSERT_TRUE( source, "Attempted to copy data from a nullptr." );
#ifdef __CUDACC__
   if constexpr( std::is_same_v< std::remove_cv_t< DestinationElement >, std::remove_cv_t< SourceElement > > ) {
      if( cudaMemcpy( destination, source, size * sizeof( DestinationElement ), cudaMemcpyDeviceToHost ) != cudaSuccess )
         std::cerr << "Transfer of data from CUDA device to host failed." << std::endl;
      TNL_CHECK_CUDA_DEVICE;
   }
   else {
      using BaseType = std::remove_cv_t< SourceElement >;
      const int buffer_size = noa::TNL::min( Cuda::getTransferBufferSize() / sizeof( BaseType ), size );
      std::unique_ptr< BaseType[] > buffer{ new BaseType[ buffer_size ] };
      Index i = 0;
      while( i < size ) {
         if( cudaMemcpy( (void*) buffer.get(),
                         (void*) &source[ i ],
                         noa::TNL::min( size - i, buffer_size ) * sizeof( SourceElement ),
                         cudaMemcpyDeviceToHost )
             != cudaSuccess )
            std::cerr << "Transfer of data from CUDA device to host failed." << std::endl;
         TNL_CHECK_CUDA_DEVICE;
         int j = 0;
         while( j < buffer_size && i + j < size ) {
            destination[ i + j ] = buffer[ j ];
            j++;
         }
         i += j;
      }
   }
#else
   throw Exceptions::CudaSupportMissing();
#endif
}

template< typename DeviceType >
template< typename DestinationElement, typename SourceElement, typename Index >
void
Copy< Devices::Cuda, DeviceType >::copy( DestinationElement* destination, const SourceElement* source, Index size )
{
   if( size == 0 )
      return;
   TNL_ASSERT_TRUE( destination, "Attempted to copy data to a nullptr." );
   TNL_ASSERT_TRUE( source, "Attempted to copy data from a nullptr." );
   TNL_ASSERT_GE( size, (Index) 0, "Array size must be non-negative." );
#ifdef __CUDACC__
   if constexpr( std::is_same_v< std::remove_cv_t< DestinationElement >, std::remove_cv_t< SourceElement > > ) {
      if( cudaMemcpy( destination, source, size * sizeof( DestinationElement ), cudaMemcpyHostToDevice ) != cudaSuccess )
         std::cerr << "Transfer of data from host to CUDA device failed." << std::endl;
      TNL_CHECK_CUDA_DEVICE;
   }
   else {
      const int buffer_size = noa::TNL::min( Cuda::getTransferBufferSize() / sizeof( DestinationElement ), size );
      std::unique_ptr< DestinationElement[] > buffer{ new DestinationElement[ buffer_size ] };
      Index i = 0;
      while( i < size ) {
         int j = 0;
         while( j < buffer_size && i + j < size ) {
            buffer[ j ] = source[ i + j ];
            j++;
         }
         if( cudaMemcpy(
                (void*) &destination[ i ], (void*) buffer.get(), j * sizeof( DestinationElement ), cudaMemcpyHostToDevice )
             != cudaSuccess )
            std::cerr << "Transfer of data from host to CUDA device failed." << std::endl;
         TNL_CHECK_CUDA_DEVICE;
         i += j;
      }
   }
#else
   throw Exceptions::CudaSupportMissing();
#endif
}

}  // namespace noa::TNL::Algorithms::detail
