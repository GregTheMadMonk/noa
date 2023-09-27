// Copyright (c) 2004-2023 Tomáš Oberhuber et al.
//
// This file is part of TNL - Template Numerical Library (https://tnl-project.org/)
//
// SPDX-License-Identifier: MIT

#pragma once

#include <type_traits>
#include <noa/3rdparty/tnl-noa/src/TNL/Containers/Vector.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Algorithms/Segments/BiEllpackSegmentView.h>

namespace noa::TNL::Algorithms::Segments::detail {

template< typename Index, typename Device, ElementsOrganization Organization, int WarpSize >
class BiEllpack
{
public:
   using DeviceType = Device;
   using IndexType = Index;
   using OffsetsContainer = Containers::Vector< IndexType, DeviceType, IndexType >;
   using OffsetsHolderView = typename OffsetsContainer::ConstViewType;
   using ConstOffsetsHolderView = typename OffsetsHolderView::ConstViewType;
   using SegmentViewType = BiEllpackSegmentView< IndexType, Organization >;

   [[nodiscard]] static constexpr int
   getWarpSize()
   {
      return WarpSize;
   }

   [[nodiscard]] static constexpr int
   getLogWarpSize()
   {
      return noa::TNL::discreteLog2( WarpSize );
   }

   [[nodiscard]] static constexpr int
   getGroupsCount()
   {
      return getLogWarpSize() + 1;
   }

   [[nodiscard]] __cuda_callable__
   static IndexType
   getActiveGroupsCountDirect( const ConstOffsetsHolderView& rowPermArray, const IndexType segmentIdx )
   {
      TNL_ASSERT_GE( segmentIdx, 0, "" );
      // TNL_ASSERT_LT( segmentIdx, this->getSize(), "" );

      IndexType strip = segmentIdx / getWarpSize();
      IndexType rowStripPermutation = rowPermArray[ segmentIdx ] - getWarpSize() * strip;
      IndexType numberOfGroups = getLogWarpSize() + 1;
      IndexType bisection = 1;
      for( IndexType i = 0; i < getLogWarpSize() + 1; i++ ) {
         if( rowStripPermutation < bisection )
            return numberOfGroups - i;
         bisection *= 2;
      }
      TNL_ASSERT_TRUE( false, "segmentIdx was not found" );
      return -1;  // to avoid compiler warning
   }

   [[nodiscard]] static IndexType
   getActiveGroupsCount( const ConstOffsetsHolderView& rowPermArray, const IndexType segmentIdx )
   {
      TNL_ASSERT_GE( segmentIdx, 0, "" );
      // TNL_ASSERT_LT( segmentIdx, this->getSize(), "" );

      IndexType strip = segmentIdx / getWarpSize();
      IndexType rowStripPermutation = rowPermArray.getElement( segmentIdx ) - getWarpSize() * strip;
      IndexType numberOfGroups = getLogWarpSize() + 1;
      IndexType bisection = 1;
      for( IndexType i = 0; i < getLogWarpSize() + 1; i++ ) {
         if( rowStripPermutation < bisection )
            return numberOfGroups - i;
         bisection *= 2;
      }
      throw std::logic_error( "segmentIdx was not found" );
   }

   [[nodiscard]] __cuda_callable__
   static IndexType
   getGroupSizeDirect( const ConstOffsetsHolderView& groupPointers, const IndexType strip, const IndexType group )
   {
      const IndexType groupOffset = strip * ( getLogWarpSize() + 1 ) + group;
      return groupPointers[ groupOffset + 1 ] - groupPointers[ groupOffset ];
   }

   [[nodiscard]] static IndexType
   getGroupSize( const ConstOffsetsHolderView& groupPointers, const IndexType strip, const IndexType group )
   {
      const IndexType groupOffset = strip * ( getLogWarpSize() + 1 ) + group;
      return groupPointers.getElement( groupOffset + 1 ) - groupPointers.getElement( groupOffset );
   }

   [[nodiscard]] __cuda_callable__
   static IndexType
   getSegmentSizeDirect( const OffsetsHolderView& rowPermArray,
                         const OffsetsHolderView& groupPointers,
                         const IndexType segmentIdx )
   {
      const IndexType strip = segmentIdx / getWarpSize();
      const IndexType groupsCount = getActiveGroupsCountDirect( rowPermArray, segmentIdx );
      IndexType groupHeight = getWarpSize();
      IndexType segmentSize = 0;
      for( IndexType group = 0; group < groupsCount; group++ ) {
         const IndexType groupSize = getGroupSizeDirect( groupPointers, strip, group );
         IndexType groupWidth = groupSize / groupHeight;
         segmentSize += groupWidth;
         groupHeight /= 2;
      }
      return segmentSize;
   }

   [[nodiscard]] static IndexType
   getSegmentSize( const OffsetsHolderView& rowPermArray, const OffsetsHolderView& groupPointers, const IndexType segmentIdx )
   {
      const IndexType strip = segmentIdx / getWarpSize();
      const IndexType groupsCount = getActiveGroupsCount( rowPermArray, segmentIdx );
      IndexType groupHeight = getWarpSize();
      IndexType segmentSize = 0;
      for( IndexType group = 0; group < groupsCount; group++ ) {
         const IndexType groupSize = getGroupSize( groupPointers, strip, group );
         IndexType groupWidth = groupSize / groupHeight;
         segmentSize += groupWidth;
         groupHeight /= 2;
      }
      return segmentSize;
   }

   [[nodiscard]] __cuda_callable__
   static IndexType
   getGlobalIndexDirect( const OffsetsHolderView& rowPermArray,
                         const OffsetsHolderView& groupPointers,
                         const IndexType segmentIdx,
                         IndexType localIdx )
   {
      const IndexType strip = segmentIdx / getWarpSize();
      const IndexType groupIdx = strip * ( getLogWarpSize() + 1 );
      const IndexType rowStripPerm = rowPermArray[ segmentIdx ] - strip * getWarpSize();
      const IndexType groupsCount = getActiveGroupsCountDirect( rowPermArray, segmentIdx );
      IndexType globalIdx = groupPointers[ groupIdx ];
      IndexType groupHeight = getWarpSize();
      for( IndexType group = 0; group < groupsCount; group++ ) {
         const IndexType groupSize = getGroupSizeDirect( groupPointers, strip, group );
         if( groupSize ) {
            IndexType groupWidth = groupSize / groupHeight;
            if( localIdx >= groupWidth ) {
               localIdx -= groupWidth;
               globalIdx += groupSize;
            }
            else {
               if( Organization == RowMajorOrder )
                  return globalIdx + rowStripPerm * groupWidth + localIdx;
               else
                  return globalIdx + rowStripPerm + localIdx * groupHeight;
            }
         }
         groupHeight /= 2;
      }
      TNL_ASSERT_TRUE( false, "segment capacity exceeded, wrong localIdx" );
      return -1;  // to avoid compiler warning
   }

   [[nodiscard]] static IndexType
   getGlobalIndex( const ConstOffsetsHolderView& rowPermArray,
                   const ConstOffsetsHolderView& groupPointers,
                   const IndexType segmentIdx,
                   IndexType localIdx )
   {
      const IndexType strip = segmentIdx / getWarpSize();
      const IndexType groupIdx = strip * ( getLogWarpSize() + 1 );
      const IndexType rowStripPerm = rowPermArray.getElement( segmentIdx ) - strip * getWarpSize();
      const IndexType groupsCount = getActiveGroupsCount( rowPermArray, segmentIdx );
      IndexType globalIdx = groupPointers.getElement( groupIdx );
      IndexType groupHeight = getWarpSize();
      for( IndexType group = 0; group < groupsCount; group++ ) {
         const IndexType groupSize = getGroupSize( groupPointers, strip, group );
         if( groupSize ) {
            IndexType groupWidth = groupSize / groupHeight;
            if( localIdx >= groupWidth ) {
               localIdx -= groupWidth;
               globalIdx += groupSize;
            }
            else {
               if( Organization == RowMajorOrder ) {
                  return globalIdx + rowStripPerm * groupWidth + localIdx;
               }
               else
                  return globalIdx + rowStripPerm + localIdx * groupHeight;
            }
         }
         groupHeight /= 2;
      }
      throw std::logic_error( "segment capacity exceeded, wrong localIdx" );
   }

   [[nodiscard]] __cuda_callable__
   static SegmentViewType
   getSegmentViewDirect( const OffsetsHolderView& rowPermArray,
                         const OffsetsHolderView& groupPointers,
                         const IndexType segmentIdx )
   {
      using GroupsWidthType = typename SegmentViewType::GroupsWidthType;

      const IndexType strip = segmentIdx / getWarpSize();
      const IndexType groupIdx = strip * ( getLogWarpSize() + 1 );
      const IndexType inStripIdx = rowPermArray[ segmentIdx ] - strip * getWarpSize();
      const IndexType groupsCount = getActiveGroupsCountDirect( rowPermArray, segmentIdx );
      IndexType groupHeight = getWarpSize();
      GroupsWidthType groupsWidth( 0 );
      TNL_ASSERT_LE( groupsCount, getGroupsCount(), "" );
      for( IndexType i = 0; i < groupsCount; i++ ) {
         const IndexType groupSize = groupPointers[ groupIdx + i + 1 ] - groupPointers[ groupIdx + i ];
         groupsWidth[ i ] = groupSize / groupHeight;
         groupHeight /= 2;
         // std::cerr << " ROW INIT: groupIdx = " << i << " groupSize = " << groupSize << " groupWidth = " << groupsWidth[ i ]
         // << std::endl;
      }
      return { segmentIdx, groupPointers[ groupIdx ], inStripIdx, groupsWidth };
   }

   [[nodiscard]] __cuda_callable__
   static SegmentViewType
   getSegmentView( const OffsetsHolderView& rowPermArray, const OffsetsHolderView& groupPointers, const IndexType segmentIdx )
   {
      using GroupsWidthType = typename SegmentViewType::GroupsWidthType;

      const IndexType strip = segmentIdx / getWarpSize();
      const IndexType groupIdx = strip * ( getLogWarpSize() + 1 );
      const IndexType inStripIdx = rowPermArray.getElement( segmentIdx ) - strip * getWarpSize();
      const IndexType groupsCount = getActiveGroupsCount( rowPermArray, segmentIdx );
      IndexType groupHeight = getWarpSize();
      GroupsWidthType groupsWidth( 0 );
      for( IndexType i = 0; i < groupsCount; i++ ) {
         const IndexType groupSize = groupPointers.getElement( groupIdx + i + 1 ) - groupPointers.getElement( groupIdx + i );
         groupsWidth[ i ] = groupSize / groupHeight;
         groupHeight /= 2;
      }
      return { segmentIdx, groupPointers[ groupIdx ], inStripIdx, groupsWidth };
   }
};

}  // namespace noa::TNL::Algorithms::Segments::detail
