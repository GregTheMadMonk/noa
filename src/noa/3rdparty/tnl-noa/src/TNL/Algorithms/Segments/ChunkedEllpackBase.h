// Copyright (c) 2004-2023 Tomáš Oberhuber et al.
//
// This file is part of TNL - Template Numerical Library (https://tnl-project.org/)
//
// SPDX-License-Identifier: MIT

#pragma once

#include <type_traits>

#include <noa/3rdparty/tnl-noa/src/TNL/Containers/VectorView.h>

#include "ElementsOrganization.h"
#include "ChunkedEllpackSegmentView.h"
#include "detail/ChunkedEllpack.h"
#include "printSegments.h"

namespace noa::TNL::Algorithms::Segments {

template< typename Device, typename Index, ElementsOrganization Organization >
class ChunkedEllpackBase
{
public:
   using DeviceType = Device;

   using IndexType = std::remove_const_t< Index >;

   using OffsetsView = Containers::VectorView< Index, DeviceType, IndexType >;

   using ConstOffsetsView = typename OffsetsView::ConstViewType;

   template< typename Device_, typename Index_ >
   using ViewTemplate = ChunkedEllpackBase< Device_, Index_, Organization >;

   using SegmentViewType = ChunkedEllpackSegmentView< IndexType, Organization >;

   using SliceInfoType = detail::ChunkedEllpackSliceInfo< IndexType >;
   using SliceInfoContainerView =
      Containers::ArrayView< typename noa::TNL::copy_const< SliceInfoType >::template from< Index >::type, DeviceType, IndexType >;
   using ConstSliceInfoContainerView = typename SliceInfoContainerView::ConstViewType;

   [[nodiscard]] static constexpr ElementsOrganization
   getOrganization()
   {
      return Organization;
   }

   [[nodiscard]] static constexpr bool
   havePadding()
   {
      return true;
   }

   __cuda_callable__
   ChunkedEllpackBase() = default;

   __cuda_callable__
   ChunkedEllpackBase( IndexType size,
                       IndexType storageSize,
                       IndexType numberOfSlices,
                       IndexType chunksInSlice,
                       IndexType desiredChunkSize,
                       OffsetsView rowToChunkMapping,
                       OffsetsView rowToSliceMapping,
                       OffsetsView chunksToSegmentsMapping,
                       OffsetsView rowPointers,
                       SliceInfoContainerView slices );

   __cuda_callable__
   ChunkedEllpackBase( const ChunkedEllpackBase& ) = default;

   __cuda_callable__
   ChunkedEllpackBase( ChunkedEllpackBase&& ) noexcept = default;

   ChunkedEllpackBase&
   operator=( const ChunkedEllpackBase& ) = delete;

   ChunkedEllpackBase&
   operator=( ChunkedEllpackBase&& ) = delete;

   [[nodiscard]] static std::string
   getSerializationType();

   [[nodiscard]] static std::string
   getSegmentsType();

   [[nodiscard]] __cuda_callable__
   IndexType
   getSegmentsCount() const;

   [[nodiscard]] __cuda_callable__
   IndexType
   getSegmentSize( IndexType segmentIdx ) const;

   [[nodiscard]] __cuda_callable__
   IndexType
   getSize() const;

   [[nodiscard]] __cuda_callable__
   IndexType
   getStorageSize() const;

   [[nodiscard]] __cuda_callable__
   IndexType
   getGlobalIndex( IndexType segmentIdx, IndexType localIdx ) const;

   [[nodiscard]] __cuda_callable__
   SegmentViewType
   getSegmentView( IndexType segmentIdx ) const;

   [[nodiscard]] __cuda_callable__
   OffsetsView
   getRowToChunkMappingView();

   [[nodiscard]] __cuda_callable__
   ConstOffsetsView
   getRowToChunkMappingView() const;

   [[nodiscard]] __cuda_callable__
   OffsetsView
   getRowToSliceMappingView();

   [[nodiscard]] __cuda_callable__
   ConstOffsetsView
   getRowToSliceMappingView() const;

   [[nodiscard]] __cuda_callable__
   OffsetsView
   getChunksToSegmentsMappingView();

   [[nodiscard]] __cuda_callable__
   ConstOffsetsView
   getChunksToSegmentsMappingView() const;

   [[nodiscard]] __cuda_callable__
   OffsetsView
   getRowPointersView();

   [[nodiscard]] __cuda_callable__
   ConstOffsetsView
   getRowPointersView() const;

   [[nodiscard]] __cuda_callable__
   SliceInfoContainerView
   getSlicesView();

   [[nodiscard]] __cuda_callable__
   ConstSliceInfoContainerView
   getSlicesView() const;

   [[nodiscard]] __cuda_callable__
   IndexType
   getNumberOfSlices() const;

   [[nodiscard]] __cuda_callable__
   IndexType
   getChunksInSlice() const;

   [[nodiscard]] __cuda_callable__
   IndexType
   getDesiredChunkSize() const;

   template< typename Function >
   void
   forElements( IndexType begin, IndexType end, Function&& function ) const;

   template< typename Function >
   void
   forAllElements( Function&& function ) const;

   template< typename Function >
   void
   forSegments( IndexType begin, IndexType end, Function&& function ) const;

   template< typename Function >
   void
   forAllSegments( Function&& function ) const;

   // TODO: sequentialForSegments, sequentialForAllSegments

   void
   printStructure( std::ostream& str ) const;

protected:
   IndexType size = 0;
   IndexType storageSize = 0;
   IndexType numberOfSlices = 0;
   IndexType chunksInSlice = 256;
   IndexType desiredChunkSize = 16;

   //! \brief For each row, this keeps index of the first chunk within a slice.
   OffsetsView rowToChunkMapping;

   //! \brief For each segment, this keeps index of the slice which contains the segment.
   OffsetsView rowToSliceMapping;

   OffsetsView chunksToSegmentsMapping;

   //! \brief Keeps index of the first segment index.
   OffsetsView rowPointers;

   SliceInfoContainerView slices;

   /**
    * \brief Re-initializes the internal attributes of the base class.
    *
    * Note that this function is \e protected to ensure that the user cannot
    * modify the base class of segments. For the same reason, in future code
    * development we also need to make sure that all non-const functions in
    * the base class return by value and not by reference.
    */
   __cuda_callable__
   void
   bind( IndexType size,
         IndexType storageSize,
         IndexType numberOfSlices,
         IndexType chunksInSlice,
         IndexType desiredChunkSize,
         OffsetsView rowToChunkMapping,
         OffsetsView rowToSliceMapping,
         OffsetsView chunksToSegmentsMapping,
         OffsetsView rowPointers,
         SliceInfoContainerView slices );
};

template< typename Device, typename Index, ElementsOrganization Organization >
std::ostream&
operator<<( std::ostream& str, const ChunkedEllpackBase< Device, Index, Organization >& segments )
{
   return printSegments( str, segments );
}

}  // namespace noa::TNL::Algorithms::Segments

#include "ChunkedEllpackBase.hpp"
