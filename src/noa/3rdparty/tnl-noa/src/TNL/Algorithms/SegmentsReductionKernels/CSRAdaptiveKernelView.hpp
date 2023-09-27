// Copyright (c) 2004-2023 Tomáš Oberhuber et al.
//
// This file is part of TNL - Template Numerical Library (https://tnl-project.org/)
//
// SPDX-License-Identifier: MIT

#pragma once

#include <noa/3rdparty/tnl-noa/src/TNL/Assert.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Cuda/LaunchHelpers.h>

#include "CSRScalarKernel.h"
#include "CSRAdaptiveKernelView.h"

namespace noa::TNL::Algorithms::SegmentsReductionKernels {

template< typename BlocksView,
          typename Offsets,
          typename Index,
          typename Fetch,
          typename Reduction,
          typename ResultKeeper,
          typename Value >
__global__
void
reduceSegmentsCSRAdaptiveKernel( BlocksView blocks,
                                 int gridIdx,
                                 Offsets offsets,
                                 Fetch fetch,
                                 Reduction reduce,
                                 ResultKeeper keep,
                                 Value identity )
{
#ifdef __CUDACC__
   using ReturnType = typename detail::FetchLambdaAdapter< Index, Fetch >::ReturnType;
   using BlockType = detail::CSRAdaptiveKernelBlockDescriptor< Index >;
   constexpr int CudaBlockSize = detail::CSRAdaptiveKernelParameters< sizeof( ReturnType ) >::CudaBlockSize();
   constexpr int WarpSize = Cuda::getWarpSize();
   constexpr int WarpsCount = detail::CSRAdaptiveKernelParameters< sizeof( ReturnType ) >::WarpsCount();
   constexpr size_t StreamedSharedElementsPerWarp =
      detail::CSRAdaptiveKernelParameters< sizeof( ReturnType ) >::StreamedSharedElementsPerWarp();

   __shared__ ReturnType streamShared[ WarpsCount ][ StreamedSharedElementsPerWarp ];
   __shared__ ReturnType multivectorShared[ CudaBlockSize / WarpSize ];
   //__shared__ BlockType sharedBlocks[ WarpsCount ];

   const Index index = ( ( gridIdx * noa::TNL::Cuda::getMaxGridXSize() + blockIdx.x ) * blockDim.x ) + threadIdx.x;
   const Index blockIdx = index / WarpSize;
   if( blockIdx >= blocks.getSize() - 1 )
      return;

   if( threadIdx.x < CudaBlockSize / WarpSize )
      multivectorShared[ threadIdx.x ] = identity;
   __syncthreads();
   ReturnType result = identity;
   bool compute = true;
   const Index laneIdx = threadIdx.x & 31;  // & is cheaper than %
   /*if( laneIdx == 0 )
      sharedBlocks[ warpIdx ] = blocks[ blockIdx ];
   __syncthreads();
   const auto& block = sharedBlocks[ warpIdx ];*/
   const BlockType block = blocks[ blockIdx ];
   const Index firstSegmentIdx = block.getFirstSegment();
   const Index begin = offsets[ firstSegmentIdx ];

   if( block.getType() == detail::Type::STREAM )  // Stream kernel - many short segments per warp
   {
      const Index warpIdx = threadIdx.x / 32;
      const Index end = begin + block.getSize();

      // Stream data to shared memory
      for( Index globalIdx = laneIdx + begin; globalIdx < end; globalIdx += WarpSize )
         streamShared[ warpIdx ][ globalIdx - begin ] = fetch( globalIdx, compute );
      __syncwarp();
      const Index lastSegmentIdx = firstSegmentIdx + block.getSegmentsInBlock();

      for( Index i = firstSegmentIdx + laneIdx; i < lastSegmentIdx; i += WarpSize ) {
         const Index sharedEnd = offsets[ i + 1 ] - begin;  // end of preprocessed data
         result = identity;
         // Scalar reduction
         for( Index sharedIdx = offsets[ i ] - begin; sharedIdx < sharedEnd; sharedIdx++ )
            result = reduce( result, streamShared[ warpIdx ][ sharedIdx ] );
         keep( i, result );
      }
   }
   else if( block.getType() == detail::Type::VECTOR )  // Vector kernel - one segment per warp
   {
      const Index end = begin + block.getSize();
      const Index segmentIdx = block.getFirstSegment();

      for( Index globalIdx = begin + laneIdx; globalIdx < end; globalIdx += WarpSize )
         result = reduce( result, fetch( globalIdx, compute ) );

      // Parallel reduction
      result = reduce( result, __shfl_down_sync( 0xFFFFFFFF, result, 16 ) );
      result = reduce( result, __shfl_down_sync( 0xFFFFFFFF, result, 8 ) );
      result = reduce( result, __shfl_down_sync( 0xFFFFFFFF, result, 4 ) );
      result = reduce( result, __shfl_down_sync( 0xFFFFFFFF, result, 2 ) );
      result = reduce( result, __shfl_down_sync( 0xFFFFFFFF, result, 1 ) );
      if( laneIdx == 0 )
         keep( segmentIdx, result );
   }
   else  // block.getType() == Type::LONG - several warps per segment
   {
      const Index segmentIdx = block.getFirstSegment();  // block.index[0];
      const Index end = offsets[ segmentIdx + 1 ];

      TNL_ASSERT_GT( block.getWarpsCount(), 0, "" );
      result = identity;
      for( Index globalIdx = begin + laneIdx + noa::TNL::Cuda::getWarpSize() * block.getWarpIdx(); globalIdx < end;
           globalIdx += noa::TNL::Cuda::getWarpSize() * block.getWarpsCount() )
      {
         result = reduce( result, fetch( globalIdx, compute ) );
      }

      result += __shfl_down_sync( 0xFFFFFFFF, result, 16 );
      result += __shfl_down_sync( 0xFFFFFFFF, result, 8 );
      result += __shfl_down_sync( 0xFFFFFFFF, result, 4 );
      result += __shfl_down_sync( 0xFFFFFFFF, result, 2 );
      result += __shfl_down_sync( 0xFFFFFFFF, result, 1 );

      const Index warpID = threadIdx.x / 32;
      if( laneIdx == 0 )
         multivectorShared[ warpID ] = result;

      __syncthreads();
      // Reduction in multivectorShared
      if( block.getWarpIdx() == 0 && laneIdx < 16 ) {
         constexpr int totalWarps = CudaBlockSize / WarpSize;
         if( totalWarps >= 32 ) {
            multivectorShared[ laneIdx ] = reduce( multivectorShared[ laneIdx ], multivectorShared[ laneIdx + 16 ] );
            __syncwarp();
         }
         if( totalWarps >= 16 ) {
            multivectorShared[ laneIdx ] = reduce( multivectorShared[ laneIdx ], multivectorShared[ laneIdx + 8 ] );
            __syncwarp();
         }
         if( totalWarps >= 8 ) {
            multivectorShared[ laneIdx ] = reduce( multivectorShared[ laneIdx ], multivectorShared[ laneIdx + 4 ] );
            __syncwarp();
         }
         if( totalWarps >= 4 ) {
            multivectorShared[ laneIdx ] = reduce( multivectorShared[ laneIdx ], multivectorShared[ laneIdx + 2 ] );
            __syncwarp();
         }
         if( totalWarps >= 2 ) {
            multivectorShared[ laneIdx ] = reduce( multivectorShared[ laneIdx ], multivectorShared[ laneIdx + 1 ] );
            __syncwarp();
         }
         if( laneIdx == 0 ) {
            // printf( "Long: segmentIdx %d -> %d \n", segmentIdx, multivectorShared[ 0 ] );
            keep( segmentIdx, multivectorShared[ 0 ] );
         }
      }
   }
#endif
}

template< typename Index, typename Device >
void
CSRAdaptiveKernelView< Index, Device >::setBlocks( BlocksType& blocks, const int idx )
{
   this->blocksArray[ idx ].bind( blocks );
}

template< typename Index, typename Device >
__cuda_callable__
auto
CSRAdaptiveKernelView< Index, Device >::getView() -> ViewType
{
   return *this;
}

template< typename Index, typename Device >
__cuda_callable__
auto
CSRAdaptiveKernelView< Index, Device >::getConstView() const -> ConstViewType
{
   return *this;
}

template< typename Index, typename Device >
std::string
CSRAdaptiveKernelView< Index, Device >::getKernelType()
{
   return "Adaptive";
}

template< typename Index, typename Device >
template< typename SegmentsView, typename Fetch, typename Reduction, typename ResultKeeper, typename Value >
void
CSRAdaptiveKernelView< Index, Device >::reduceSegments( const SegmentsView& segments,
                                                        Index begin,
                                                        Index end,
                                                        Fetch& fetch,
                                                        const Reduction& reduction,
                                                        ResultKeeper& keeper,
                                                        const Value& identity ) const
{
   int valueSizeLog = getSizeValueLog( sizeof( Value ) );

   if( valueSizeLog >= MaxValueSizeLog ) {
      noa::TNL::Algorithms::SegmentsReductionKernels::CSRScalarKernel< Index, Device >::reduceSegments(
         segments, begin, end, fetch, reduction, keeper, identity );
      return;
   }

   constexpr bool DispatchScalarCSR =
      detail::CheckFetchLambda< Index, Fetch >::hasAllParameters() || std::is_same< Device, Devices::Host >::value;
   if constexpr( DispatchScalarCSR ) {
      noa::TNL::Algorithms::SegmentsReductionKernels::CSRScalarKernel< Index, Device >::reduceSegments(
         segments, begin, end, fetch, reduction, keeper, identity );
   }
   else {
      using ReturnType = typename detail::FetchLambdaAdapter< Index, Fetch >::ReturnType;
      Devices::Cuda::LaunchConfiguration launch_config;
      launch_config.blockSize.x = detail::CSRAdaptiveKernelParameters< sizeof( ReturnType ) >::CudaBlockSize();
      constexpr std::size_t maxGridSize = noa::TNL::Cuda::getMaxGridXSize();

      // Fill blocks
      const auto& blocks = this->blocksArray[ valueSizeLog ];
      std::size_t neededThreads = blocks.getSize() * noa::TNL::Cuda::getWarpSize();  // one warp per block

      // Execute kernels on device
      for( Index gridIdx = 0; neededThreads != 0; gridIdx++ ) {
         if( maxGridSize * launch_config.blockSize.x >= neededThreads ) {
            launch_config.gridSize.x = roundUpDivision( neededThreads, launch_config.blockSize.x );
            neededThreads = 0;
         }
         else {
            launch_config.gridSize.x = maxGridSize;
            neededThreads -= maxGridSize * launch_config.blockSize.x;
         }

         using OffsetsView = typename SegmentsView::ConstOffsetsView;
         OffsetsView offsets = segments.getOffsets();

         constexpr auto kernel =
            reduceSegmentsCSRAdaptiveKernel< BlocksView, OffsetsView, Index, Fetch, Reduction, ResultKeeper, Value >;
         Cuda::launchKernelAsync( kernel, launch_config, blocks, gridIdx, offsets, fetch, reduction, keeper, identity );
      }
      cudaStreamSynchronize( launch_config.stream );
      TNL_CHECK_CUDA_DEVICE;
   }
}

template< typename Index, typename Device >
template< typename SegmentsView, typename Fetch, typename Reduction, typename ResultKeeper, typename Value >
void
CSRAdaptiveKernelView< Index, Device >::reduceAllSegments( const SegmentsView& segments,
                                                           Fetch& fetch,
                                                           const Reduction& reduction,
                                                           ResultKeeper& keeper,
                                                           const Value& identity ) const
{
   reduceSegments( segments, 0, segments.getSegmentsCount(), fetch, reduction, keeper, identity );
}

template< typename Index, typename Device >
void
CSRAdaptiveKernelView< Index, Device >::printBlocks( int idx ) const
{
   auto& blocks = this->blocksArray[ idx ];
   for( Index i = 0; i < this->blocks.getSize(); i++ ) {
      auto block = blocks.getElement( i );
      std::cout << "Block " << i << " : " << block << std::endl;
   }
}

}  // namespace noa::TNL::Algorithms::SegmentsReductionKernels
