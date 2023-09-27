// Copyright (c) 2004-2023 Tomáš Oberhuber et al.
//
// This file is part of TNL - Template Numerical Library (https://tnl-project.org/)
//
// SPDX-License-Identifier: MIT

#pragma once

#include <noa/3rdparty/tnl-noa/src/TNL/Algorithms/parallelFor.h>

#include "EllpackBase.h"

namespace noa::TNL::Algorithms::Segments {

template< typename Device, typename Index, ElementsOrganization Organization, int Alignment >
__cuda_callable__
void
EllpackBase< Device, Index, Organization, Alignment >::bind( IndexType segmentsCount,
                                                             IndexType segmentSize,
                                                             IndexType alignedSize )
{
   this->segmentSize = segmentSize;
   this->segmentsCount = segmentsCount;
   this->alignedSize = alignedSize;
}

template< typename Device, typename Index, ElementsOrganization Organization, int Alignment >
__cuda_callable__
EllpackBase< Device, Index, Organization, Alignment >::EllpackBase( IndexType segmentsCount,
                                                                    IndexType segmentSize,
                                                                    IndexType alignedSize )
: segmentSize( segmentSize ), segmentsCount( segmentsCount ), alignedSize( alignedSize )
{}

template< typename Device, typename Index, ElementsOrganization Organization, int Alignment >
std::string
EllpackBase< Device, Index, Organization, Alignment >::getSerializationType()
{
   return "Ellpack< " + noa::TNL::getSerializationType< IndexType >() + ", " + noa::TNL::getSerializationType( Organization ) + ", "
        + std::to_string( Alignment ) + " >";
}

template< typename Device, typename Index, ElementsOrganization Organization, int Alignment >
std::string
EllpackBase< Device, Index, Organization, Alignment >::getSegmentsType()
{
   return "Ellpack";
}

template< typename Device, typename Index, ElementsOrganization Organization, int Alignment >
__cuda_callable__
auto
EllpackBase< Device, Index, Organization, Alignment >::getSegmentsCount() const -> IndexType
{
   return this->segmentsCount;
}

template< typename Device, typename Index, ElementsOrganization Organization, int Alignment >
__cuda_callable__
auto
EllpackBase< Device, Index, Organization, Alignment >::getSegmentSize( const IndexType segmentIdx ) const -> IndexType
{
   return this->segmentSize;
}

template< typename Device, typename Index, ElementsOrganization Organization, int Alignment >
__cuda_callable__
auto
EllpackBase< Device, Index, Organization, Alignment >::getSize() const -> IndexType
{
   return this->segmentsCount * this->segmentSize;
}

template< typename Device, typename Index, ElementsOrganization Organization, int Alignment >
__cuda_callable__
auto
EllpackBase< Device, Index, Organization, Alignment >::getStorageSize() const -> IndexType
{
   return this->alignedSize * this->segmentSize;
}

template< typename Device, typename Index, ElementsOrganization Organization, int Alignment >
__cuda_callable__
auto
EllpackBase< Device, Index, Organization, Alignment >::getGlobalIndex( const Index segmentIdx, const Index localIdx ) const
   -> IndexType
{
   if constexpr( Organization == RowMajorOrder )
      return segmentIdx * this->segmentSize + localIdx;
   else
      return segmentIdx + this->alignedSize * localIdx;
}

template< typename Device, typename Index, ElementsOrganization Organization, int Alignment >
__cuda_callable__
auto
EllpackBase< Device, Index, Organization, Alignment >::getSegmentView( const IndexType segmentIdx ) const -> SegmentViewType
{
   if constexpr( Organization == RowMajorOrder )
      return SegmentViewType( segmentIdx, segmentIdx * this->segmentSize, this->segmentSize );
   else
      return SegmentViewType( segmentIdx, segmentIdx, this->segmentSize, this->alignedSize );
}

template< typename Device, typename Index, ElementsOrganization Organization, int Alignment >
__cuda_callable__
auto
EllpackBase< Device, Index, Organization, Alignment >::getAlignedSize() const -> IndexType
{
   return alignedSize;
}

template< typename Device, typename Index, ElementsOrganization Organization, int Alignment >
template< typename Function >
void
EllpackBase< Device, Index, Organization, Alignment >::forElements( IndexType begin, IndexType end, Function&& function ) const
{
   if constexpr( Organization == RowMajorOrder ) {
      const IndexType segmentSize = this->segmentSize;
      auto l = [ = ] __cuda_callable__( const IndexType segmentIdx ) mutable
      {
         const IndexType begin = segmentIdx * segmentSize;
         const IndexType end = begin + segmentSize;
         IndexType localIdx( 0 );
         for( IndexType globalIdx = begin; globalIdx < end; globalIdx++ )
            function( segmentIdx, localIdx++, globalIdx );
      };
      Algorithms::parallelFor< Device >( begin, end, l );
   }
   else {
      const IndexType storageSize = this->getStorageSize();
      const IndexType alignedSize = this->alignedSize;
      auto l = [ = ] __cuda_callable__( const IndexType segmentIdx ) mutable
      {
         const IndexType begin = segmentIdx;
         const IndexType end = storageSize;
         IndexType localIdx( 0 );
         for( IndexType globalIdx = begin; globalIdx < end; globalIdx += alignedSize )
            function( segmentIdx, localIdx++, globalIdx );
      };
      Algorithms::parallelFor< Device >( begin, end, l );
   }
}

template< typename Device, typename Index, ElementsOrganization Organization, int Alignment >
template< typename Function >
void
EllpackBase< Device, Index, Organization, Alignment >::forAllElements( Function&& function ) const
{
   this->forElements( 0, this->getSegmentsCount(), function );
}

template< typename Device, typename Index, ElementsOrganization Organization, int Alignment >
template< typename Function >
void
EllpackBase< Device, Index, Organization, Alignment >::forSegments( IndexType begin, IndexType end, Function&& function ) const
{
   const auto& self = *this;
   auto f = [ = ] __cuda_callable__( IndexType segmentIdx ) mutable
   {
      auto segment = self.getSegmentView( segmentIdx );
      function( segment );
   };
   Algorithms::parallelFor< DeviceType >( begin, end, f );
}

template< typename Device, typename Index, ElementsOrganization Organization, int Alignment >
template< typename Function >
void
EllpackBase< Device, Index, Organization, Alignment >::forAllSegments( Function&& function ) const
{
   this->forSegments( 0, this->getSegmentsCount(), function );
}

}  // namespace noa::TNL::Algorithms::Segments
