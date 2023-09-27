// Copyright (c) 2004-2023 Tomáš Oberhuber et al.
//
// This file is part of TNL - Template Numerical Library (https://tnl-project.org/)
//
// SPDX-License-Identifier: MIT

#pragma once

#include <type_traits>

#include <noa/3rdparty/tnl-noa/src/TNL/Containers/Vector.h>

#include "EllpackView.h"

namespace noa::TNL::Algorithms::Segments {

template< typename Device,
          typename Index,
          typename IndexAllocator = typename Allocators::Default< Device >::template Allocator< Index >,
          ElementsOrganization Organization = Segments::DefaultElementsOrganization< Device >::getOrganization(),
          int Alignment = 32 >
class Ellpack : public EllpackBase< Device, Index, Organization, Alignment >
{
   using Base = EllpackBase< Device, Index, Organization, Alignment >;

public:
   using ViewType = EllpackView< Device, Index, Organization, Alignment >;

   using ConstViewType = typename ViewType::ConstViewType;

   template< typename Device_, typename Index_ >
   using ViewTemplate = EllpackView< Device_, Index_, Organization, Alignment >;

   using OffsetsContainer = Containers::Vector< Index, Device, typename Base::IndexType, IndexAllocator >;

   Ellpack() = default;

   template< typename SizesContainer >
   Ellpack( const SizesContainer& sizes );

   template< typename ListIndex >
   Ellpack( const std::initializer_list< ListIndex >& segmentsSizes );

   Ellpack( Index segmentsCount, Index segmentSize );

   Ellpack( const Ellpack& segments ) = default;

   Ellpack( Ellpack&& segments ) noexcept = default;

   //! \brief Copy-assignment operator.
   Ellpack&
   operator=( const Ellpack& segments );

   //! \brief Move-assignment operator.
   Ellpack&
   operator=( Ellpack&& ) noexcept;

   template< typename Device_, typename Index_, typename IndexAllocator_, ElementsOrganization Organization_, int Alignment_ >
   Ellpack&
   operator=( const Ellpack< Device_, Index_, IndexAllocator_, Organization_, Alignment_ >& segments );

   [[nodiscard]] ViewType
   getView();

   [[nodiscard]] ConstViewType
   getConstView() const;

   /**
    * \brief Set sizes of particular segments.
    */
   template< typename SizesContainer >
   void
   setSegmentsSizes( const SizesContainer& sizes );

   void
   setSegmentsSizes( Index segmentsCount, Index segmentSize );

   void
   reset();

   void
   save( File& file ) const;

   void
   load( File& file );
};

}  // namespace noa::TNL::Algorithms::Segments

#include "Ellpack.hpp"
