// Copyright (c) 2004-2023 Tomáš Oberhuber et al.
//
// This file is part of TNL - Template Numerical Library (https://tnl-project.org/)
//
// SPDX-License-Identifier: MIT

#pragma once

#include <type_traits>

#include <noa/3rdparty/tnl-noa/src/TNL/Containers/VectorView.h>

#include "ElementsOrganization.h"
#include "BiEllpackSegmentView.h"
#include "printSegments.h"

namespace noa::TNL::Algorithms::Segments {

template< typename Device, typename Index, ElementsOrganization Organization, int WarpSize >
class BiEllpackBase
{
public:
   using DeviceType = Device;

   using IndexType = std::remove_const_t< Index >;

   using OffsetsView = Containers::VectorView< Index, DeviceType, IndexType >;

   using ConstOffsetsView = typename OffsetsView::ConstViewType;

   using SegmentViewType = BiEllpackSegmentView< IndexType, Organization, WarpSize >;

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
   BiEllpackBase() = default;

   __cuda_callable__
   BiEllpackBase( IndexType size, IndexType storageSize, OffsetsView rowPermArray, OffsetsView groupPointers );

   __cuda_callable__
   BiEllpackBase( const BiEllpackBase& ) = default;

   __cuda_callable__
   BiEllpackBase( BiEllpackBase&& ) noexcept = default;

   BiEllpackBase&
   operator=( const BiEllpackBase& ) = delete;

   BiEllpackBase&
   operator=( BiEllpackBase&& ) = delete;

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
   getGlobalIndex( Index segmentIdx, Index localIdx ) const;

   [[nodiscard]] __cuda_callable__
   SegmentViewType
   getSegmentView( IndexType segmentIdx ) const;

   [[nodiscard]] __cuda_callable__
   OffsetsView
   getRowPermArrayView();

   [[nodiscard]] __cuda_callable__
   ConstOffsetsView
   getRowPermArrayView() const;

   [[nodiscard]] __cuda_callable__
   OffsetsView
   getGroupPointersView();

   [[nodiscard]] __cuda_callable__
   ConstOffsetsView
   getGroupPointersView() const;

   [[nodiscard]] __cuda_callable__
   IndexType
   getVirtualRows() const;

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
   OffsetsView rowPermArray;
   OffsetsView groupPointers;

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
   bind( IndexType size, IndexType storageSize, OffsetsView rowPermArray, OffsetsView groupPointers );
};

template< typename Device, typename Index, ElementsOrganization Organization, int WarpSize >
std::ostream&
operator<<( std::ostream& str, const BiEllpackBase< Device, Index, Organization, WarpSize >& segments )
{
   return printSegments( str, segments );
}

}  // namespace noa::TNL::Algorithms::Segments

#include "BiEllpackBase.hpp"
