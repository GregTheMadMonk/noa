// Copyright (c) 2004-2023 Tomáš Oberhuber et al.
//
// This file is part of TNL - Template Numerical Library (https://tnl-project.org/)
//
// SPDX-License-Identifier: MIT

#pragma once

#include <noa/3rdparty/tnl-noa/src/TNL/Assert.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Cuda/LaunchHelpers.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Containers/VectorView.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Algorithms/Segments/detail/LambdaAdapter.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Algorithms/Segments/Kernels/CSRVectorKernel.h>

namespace noa::TNL::Algorithms::Segments {

template< typename Offsets,
          typename Index,
          typename Fetch,
          typename Reduction,
          typename ResultKeeper,
          typename Real,
          typename... Args >
__global__
void
reduceSegmentsCSRKernelVector( int gridIdx,
                               const Offsets offsets,
                               Index first,
                               Index last,
                               Fetch fetch,
                               const Reduction reduce,
                               ResultKeeper keep,
                               const Real zero,
                               Args... args )
{
#ifdef __CUDACC__
   /***
    * We map one warp to each segment
    */
   const Index segmentIdx = noa::TNL::Cuda::getGlobalThreadIdx_x( gridIdx ) / noa::TNL::Cuda::getWarpSize() + first;
   if( segmentIdx >= last )
      return;

   const int laneIdx = threadIdx.x & ( noa::TNL::Cuda::getWarpSize() - 1 );  // & is cheaper than %
   TNL_ASSERT_LT( segmentIdx + 1, offsets.getSize(), "" );
   Index endIdx = offsets[ segmentIdx + 1 ];

   Index localIdx( laneIdx );
   Real aux = zero;
   bool compute( true );
   for( Index globalIdx = offsets[ segmentIdx ] + localIdx; globalIdx < endIdx; globalIdx += noa::TNL::Cuda::getWarpSize() ) {
      TNL_ASSERT_LT( globalIdx, endIdx, "" );
      aux = reduce( aux, detail::FetchLambdaAdapter< Index, Fetch >::call( fetch, segmentIdx, localIdx, globalIdx, compute ) );
      localIdx += noa::TNL::Cuda::getWarpSize();
   }

   /****
    * Reduction in each warp which means in each segment.
    */
   aux = reduce( aux, __shfl_down_sync( 0xFFFFFFFF, aux, 16 ) );
   aux = reduce( aux, __shfl_down_sync( 0xFFFFFFFF, aux, 8 ) );
   aux = reduce( aux, __shfl_down_sync( 0xFFFFFFFF, aux, 4 ) );
   aux = reduce( aux, __shfl_down_sync( 0xFFFFFFFF, aux, 2 ) );
   aux = reduce( aux, __shfl_down_sync( 0xFFFFFFFF, aux, 1 ) );

   if( laneIdx == 0 )
      keep( segmentIdx, aux );
#endif
}

template< typename Index, typename Device >
template< typename Offsets >
void
CSRVectorKernel< Index, Device >::init( const Offsets& offsets )
{}

template< typename Index, typename Device >
void
CSRVectorKernel< Index, Device >::reset()
{}

template< typename Index, typename Device >
__cuda_callable__
auto
CSRVectorKernel< Index, Device >::getView() -> ViewType
{
   return *this;
}

template< typename Index, typename Device >
__cuda_callable__
auto
CSRVectorKernel< Index, Device >::getConstView() const -> ConstViewType
{
   return *this;
}

template< typename Index, typename Device >
TNL::String
CSRVectorKernel< Index, Device >::getKernelType()
{
   return "Vector";
}

template< typename Index, typename Device >
template< typename OffsetsView, typename Fetch, typename Reduction, typename ResultKeeper, typename Real, typename... Args >
void
CSRVectorKernel< Index, Device >::reduceSegments( const OffsetsView& offsets,
                                                  Index first,
                                                  Index last,
                                                  Fetch& fetch,
                                                  const Reduction& reduction,
                                                  ResultKeeper& keeper,
                                                  const Real& zero,
                                                  Args... args )
{
   if( last <= first )
      return;

   const Index warpsCount = last - first;
   const std::size_t threadsCount = warpsCount * noa::TNL::Cuda::getWarpSize();
   Devices::Cuda::LaunchConfiguration launch_config;
   launch_config.blockSize.x = 256;
   dim3 blocksCount, gridsCount;
   noa::TNL::Cuda::setupThreads( launch_config.blockSize, blocksCount, gridsCount, threadsCount );
   for( unsigned int gridIdx = 0; gridIdx < gridsCount.x; gridIdx++ ) {
      noa::TNL::Cuda::setupGrid( blocksCount, gridsCount, gridIdx, launch_config.gridSize );
      constexpr auto kernel =
         reduceSegmentsCSRKernelVector< OffsetsView, IndexType, Fetch, Reduction, ResultKeeper, Real, Args... >;
      Cuda::launchKernelAsync( kernel, launch_config, gridIdx, offsets, first, last, fetch, reduction, keeper, zero, args... );
   }
   cudaStreamSynchronize( launch_config.stream );
   TNL_CHECK_CUDA_DEVICE;
}

}  // namespace noa::TNL::Algorithms::Segments
