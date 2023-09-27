// Copyright (c) 2004-2023 Tomáš Oberhuber et al.
//
// This file is part of TNL - Template Numerical Library (https://tnl-project.org/)
//
// SPDX-License-Identifier: MIT

#pragma once

#include <type_traits>

#include <noa/3rdparty/tnl-noa/src/TNL/Containers/Vector.h>

#include "SlicedEllpackView.h"

namespace noa::TNL::Algorithms::Segments {

template< typename Device,
          typename Index,
          typename IndexAllocator = typename Allocators::Default< Device >::template Allocator< Index >,
          ElementsOrganization Organization = Algorithms::Segments::DefaultElementsOrganization< Device >::getOrganization(),
          int SliceSize = 32 >
class SlicedEllpack : public SlicedEllpackBase< Device, Index, Organization, SliceSize >
{
   using Base = SlicedEllpackBase< Device, Index, Organization, SliceSize >;

public:
   using ViewType = SlicedEllpackView< Device, Index, Organization, SliceSize >;

   using ConstViewType = SlicedEllpackView< Device, std::add_const_t< Index >, Organization, SliceSize >;

   template< typename Device_, typename Index_ >
   using ViewTemplate = SlicedEllpackView< Device_, Index_, Organization, SliceSize >;

   using OffsetsContainer = Containers::Vector< Index, Device, typename Base::IndexType, IndexAllocator >;

   SlicedEllpack() = default;

   template< typename SizesContainer >
   SlicedEllpack( const SizesContainer& segmentsSizes );

   template< typename ListIndex >
   SlicedEllpack( const std::initializer_list< ListIndex >& segmentsSizes );

   SlicedEllpack( const SlicedEllpack& );

   SlicedEllpack( SlicedEllpack&& ) noexcept = default;

   //! \brief Copy-assignment operator (makes a deep copy).
   SlicedEllpack&
   operator=( const SlicedEllpack& segments );

   //! \brief Move-assignment operator.
   SlicedEllpack&
   operator=( SlicedEllpack&& ) noexcept( false );

   template< typename Device_, typename Index_, typename IndexAllocator_, ElementsOrganization Organization_ >
   SlicedEllpack&
   operator=( const SlicedEllpack< Device_, Index_, IndexAllocator_, Organization_, SliceSize >& segments );

   [[nodiscard]] ViewType
   getView();

   [[nodiscard]] ConstViewType
   getConstView() const;

   /**
    * \brief Set sizes of particular segments.
    */
   template< typename SizesHolder = OffsetsContainer >
   void
   setSegmentsSizes( const SizesHolder& sizes );

   void
   reset();

   void
   save( File& file ) const;

   void
   load( File& file );

protected:
   OffsetsContainer sliceOffsets;
   OffsetsContainer sliceSegmentSizes;
};

}  // namespace noa::TNL::Algorithms::Segments

#include "SlicedEllpack.hpp"
