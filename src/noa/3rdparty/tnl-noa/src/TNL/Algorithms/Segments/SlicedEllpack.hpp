// Copyright (c) 2004-2023 Tomáš Oberhuber et al.
//
// This file is part of TNL - Template Numerical Library (https://tnl-project.org/)
//
// SPDX-License-Identifier: MIT

#pragma once

#include <noa/3rdparty/tnl-noa/src/TNL/Algorithms/scan.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Algorithms/SegmentsReductionKernels/EllpackKernel.h>

#include "Ellpack.h"
#include "SlicedEllpack.h"

namespace noa::TNL::Algorithms::Segments {

template< typename Device, typename Index, typename IndexAllocator, ElementsOrganization Organization, int SliceSize >
SlicedEllpack< Device, Index, IndexAllocator, Organization, SliceSize >::SlicedEllpack( const SlicedEllpack& segments )
: sliceOffsets( segments.sliceOffsets ), sliceSegmentSizes( segments.sliceSegmentSizes )
{
   // update the base
   Base::bind( segments.getSize(),
               segments.getStorageSize(),
               segments.getSegmentsCount(),
               this->sliceOffsets.getView(),
               this->sliceSegmentSizes.getView() );
}

template< typename Device, typename Index, typename IndexAllocator, ElementsOrganization Organization, int SliceSize >
template< typename SizesContainer >
SlicedEllpack< Device, Index, IndexAllocator, Organization, SliceSize >::SlicedEllpack( const SizesContainer& segmentsSizes )
{
   this->setSegmentsSizes( segmentsSizes );
}

template< typename Device, typename Index, typename IndexAllocator, ElementsOrganization Organization, int SliceSize >
template< typename ListIndex >
SlicedEllpack< Device, Index, IndexAllocator, Organization, SliceSize >::SlicedEllpack(
   const std::initializer_list< ListIndex >& segmentsSizes )
{
   this->setSegmentsSizes( OffsetsContainer( segmentsSizes ) );
}

template< typename Device, typename Index, typename IndexAllocator, ElementsOrganization Organization, int SliceSize >
SlicedEllpack< Device, Index, IndexAllocator, Organization, SliceSize >&
SlicedEllpack< Device, Index, IndexAllocator, Organization, SliceSize >::operator=( const SlicedEllpack& segments )
{
   this->sliceOffsets = segments.sliceOffsets;
   this->sliceSegmentSizes = segments.sliceSegmentSizes;
   // update the base
   Base::bind( segments.getSize(),
               segments.getStorageSize(),
               segments.getSegmentsCount(),
               this->sliceOffsets.getView(),
               this->sliceSegmentSizes.getView() );
   return *this;
}

template< typename Device, typename Index, typename IndexAllocator, ElementsOrganization Organization, int SliceSize >
SlicedEllpack< Device, Index, IndexAllocator, Organization, SliceSize >&
SlicedEllpack< Device, Index, IndexAllocator, Organization, SliceSize >::operator=( SlicedEllpack&& segments ) noexcept( false )
{
   this->sliceOffsets = std::move( segments.sliceOffsets );
   this->sliceSegmentSizes = std::move( segments.sliceSegmentSizes );
   // update the base
   Base::bind( segments.getSize(),
               segments.getStorageSize(),
               segments.getSegmentsCount(),
               this->sliceOffsets.getView(),
               this->sliceSegmentSizes.getView() );
   return *this;
}

template< typename Device, typename Index, typename IndexAllocator, ElementsOrganization Organization, int SliceSize >
template< typename Device_, typename Index_, typename IndexAllocator_, ElementsOrganization Organization_ >
SlicedEllpack< Device, Index, IndexAllocator, Organization, SliceSize >&
SlicedEllpack< Device, Index, IndexAllocator, Organization, SliceSize >::operator=(
   const SlicedEllpack< Device_, Index_, IndexAllocator_, Organization_, SliceSize >& segments )
{
   this->sliceOffsets = segments.getSliceOffsetsView();
   this->sliceSegmentSizes = segments.getSliceSegmentSizesView();
   // update the base
   Base::bind( segments.getSize(),
               segments.getStorageSize(),
               segments.getSegmentsCount(),
               this->sliceOffsets.getView(),
               this->sliceSegmentSizes.getView() );
   return *this;
}

template< typename Device, typename Index, typename IndexAllocator, ElementsOrganization Organization, int SliceSize >
typename SlicedEllpack< Device, Index, IndexAllocator, Organization, SliceSize >::ViewType
SlicedEllpack< Device, Index, IndexAllocator, Organization, SliceSize >::getView()
{
   return { this->getSize(),
            this->getStorageSize(),
            this->getSegmentsCount(),
            this->getSliceOffsetsView(),
            this->getSliceSegmentSizesView() };
}

template< typename Device, typename Index, typename IndexAllocator, ElementsOrganization Organization, int SliceSize >
auto
SlicedEllpack< Device, Index, IndexAllocator, Organization, SliceSize >::getConstView() const -> ConstViewType
{
   return { this->getSize(),
            this->getStorageSize(),
            this->getSegmentsCount(),
            this->getSliceOffsetsView(),
            this->getSliceSegmentSizesView() };
}

template< typename Device, typename Index, typename IndexAllocator, ElementsOrganization Organization, int SliceSize >
template< typename SizesHolder >
void
SlicedEllpack< Device, Index, IndexAllocator, Organization, SliceSize >::setSegmentsSizes( const SizesHolder& sizes )
{
   const Index slicesCount = roundUpDivision( sizes.getSize(), Base::getSliceSize() );
   this->sliceOffsets.setSize( slicesCount + 1 );
   this->sliceOffsets = 0;
   this->sliceSegmentSizes.setSize( slicesCount );
   Ellpack< Device, Index, IndexAllocator, RowMajorOrder > ellpack;
   ellpack.setSegmentsSizes( slicesCount, SliceSize );

   const Index size = sizes.getSize();
   const auto sizes_view = sizes.getConstView();
   auto slices_view = this->sliceOffsets.getView();
   auto slice_segment_size_view = this->sliceSegmentSizes.getView();
   auto fetch = [ = ] __cuda_callable__( Index segmentIdx, Index localIdx, Index globalIdx, bool& compute ) -> Index
   {
      if( globalIdx < size )
         return sizes_view[ globalIdx ];
      return 0;
   };
   auto reduce = [] __cuda_callable__( Index a, Index b ) -> Index
   {
      return noa::TNL::max( a, b );
   };
   auto keep = [ = ] __cuda_callable__( Index i, Index res ) mutable
   {
      slices_view[ i ] = res * SliceSize;
      slice_segment_size_view[ i ] = res;
   };
   using Kernel = SegmentsReductionKernels::EllpackKernel< Index, Device >;
   Kernel::reduceAllSegments( ellpack, fetch, reduce, keep, std::numeric_limits< Index >::min() );
   Algorithms::inplaceExclusiveScan( this->sliceOffsets );

   // update the base
   Base::bind( sum( sizes ),
               this->sliceOffsets.getElement( slicesCount ),
               sizes.getSize(),
               this->sliceOffsets.getView(),
               this->sliceSegmentSizes.getView() );
}

template< typename Device, typename Index, typename IndexAllocator, ElementsOrganization Organization, int SliceSize >
void
SlicedEllpack< Device, Index, IndexAllocator, Organization, SliceSize >::reset()
{
   this->sliceOffsets.reset();
   this->sliceSegmentSizes.reset();

   // update the base
   Base::bind( 0, 0, 0, this->sliceOffsets.getView(), this->sliceSegmentSizes.getView() );
}

template< typename Device, typename Index, typename IndexAllocator, ElementsOrganization Organization, int SliceSize >
void
SlicedEllpack< Device, Index, IndexAllocator, Organization, SliceSize >::save( File& file ) const
{
   file.save( &this->size );
   file.save( &this->storageSize );
   file.save( &this->segmentsCount );
   file << this->sliceOffsets;
   file << this->sliceSegmentSizes;
}

template< typename Device, typename Index, typename IndexAllocator, ElementsOrganization Organization, int SliceSize >
void
SlicedEllpack< Device, Index, IndexAllocator, Organization, SliceSize >::load( File& file )
{
   file.load( &this->size );
   file.load( &this->storageSize );
   file.load( &this->segmentsCount );
   file >> this->sliceOffsets;
   file >> this->sliceSegmentSizes;

   // update the base
   Base::bind(
      this->size, this->storageSize, this->segmentsCount, this->sliceOffsets.getView(), this->sliceSegmentSizes.getView() );
}

}  // namespace noa::TNL::Algorithms::Segments
