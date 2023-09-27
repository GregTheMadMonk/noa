// Copyright (c) 2004-2023 Tomáš Oberhuber et al.
//
// This file is part of TNL - Template Numerical Library (https://tnl-project.org/)
//
// SPDX-License-Identifier: MIT

#pragma once

#include "ChunkedEllpackBase.h"

namespace noa::TNL::Algorithms::Segments {

template< typename Device,
          typename Index,
          ElementsOrganization Organization = Algorithms::Segments::DefaultElementsOrganization< Device >::getOrganization() >
class ChunkedEllpackView : public ChunkedEllpackBase< Device, Index, Organization >
{
   using Base = ChunkedEllpackBase< Device, Index, Organization >;

public:
   using ViewType = ChunkedEllpackView;

   using ConstViewType = ChunkedEllpackView< Device, std::add_const_t< Index >, Organization >;

   template< typename Device_, typename Index_ >
   using ViewTemplate = ChunkedEllpackView< Device_, Index_, Organization >;

   __cuda_callable__
   ChunkedEllpackView() = default;

   __cuda_callable__
   ChunkedEllpackView( Index size,
                       Index storageSize,
                       Index numberOfSlices,
                       Index chunksInSlice,
                       Index desiredChunkSize,
                       typename Base::OffsetsView rowToChunkMapping,
                       typename Base::OffsetsView rowToSliceMapping,
                       typename Base::OffsetsView chunksToSegmentsMapping,
                       typename Base::OffsetsView rowPointers,
                       typename Base::SliceInfoContainerView slices );

   __cuda_callable__
   ChunkedEllpackView( const ChunkedEllpackView& ) = default;

   __cuda_callable__
   ChunkedEllpackView( ChunkedEllpackView&& ) noexcept = default;

   ChunkedEllpackView&
   operator=( const ChunkedEllpackView& ) = delete;

   ChunkedEllpackView&
   operator=( ChunkedEllpackView&& ) = delete;

   __cuda_callable__
   void
   bind( ChunkedEllpackView view );

   [[nodiscard]] __cuda_callable__
   ViewType
   getView();

   [[nodiscard]] __cuda_callable__
   ConstViewType
   getConstView() const;

   void
   save( File& file ) const;

   void
   load( File& file );
};

}  // namespace noa::TNL::Algorithms::Segments

#include "ChunkedEllpackView.hpp"
