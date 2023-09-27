// Copyright (c) 2004-2023 Tomáš Oberhuber et al.
//
// This file is part of TNL - Template Numerical Library (https://tnl-project.org/)
//
// SPDX-License-Identifier: MIT

#pragma once

#include "CSRAdaptiveKernelView.h"

#include "detail/FetchLambdaAdapter.h"

namespace noa::TNL::Algorithms::SegmentsReductionKernels {

template< typename Index, typename Device >
struct CSRAdaptiveKernel
{
   using IndexType = Index;
   using DeviceType = Device;
   using ViewType = CSRAdaptiveKernelView< Index, Device >;
   using ConstViewType = CSRAdaptiveKernelView< Index, Device >;
   using BlocksType = typename ViewType::BlocksType;
   using BlocksView = typename BlocksType::ViewType;

   [[nodiscard]] static constexpr int
   MaxValueSizeLog()
   {
      return ViewType::MaxValueSizeLog;
   }

   [[nodiscard]] static int
   getSizeValueLog( const int& i )
   {
      return detail::CSRAdaptiveKernelParameters<>::getSizeValueLog( i );
   }

   [[nodiscard]] static std::string
   getKernelType();

   template< typename Segments >
   void
   init( const Segments& segments );

   void
   reset();

   [[nodiscard]] __cuda_callable__
   ViewType
   getView();

   [[nodiscard]] __cuda_callable__
   ConstViewType
   getConstView() const;

   template< typename SegmentsView,
             typename Fetch,
             typename Reduction,
             typename ResultKeeper,
             typename Value = typename detail::FetchLambdaAdapter< Index, Fetch >::ReturnType >
   void
   reduceSegments( const SegmentsView& segments,
                   Index begin,
                   Index end,
                   Fetch& fetch,
                   const Reduction& reduction,
                   ResultKeeper& keeper,
                   const Value& identity = Reduction::template getIdentity< Value >() ) const;

   template< typename SegmentsView,
             typename Fetch,
             typename Reduction,
             typename ResultKeeper,
             typename Value = typename detail::FetchLambdaAdapter< Index, Fetch >::ReturnType >
   void
   reduceAllSegments( const SegmentsView& segments,
                      Fetch& fetch,
                      const Reduction& reduction,
                      ResultKeeper& keeper,
                      const Value& identity = Reduction::template getIdentity< Value >() ) const;

protected:
   template< int SizeOfValue, typename Offsets >
   Index
   findLimit( Index start, const Offsets& offsets, Index size, detail::Type& type, size_t& sum );

   template< int SizeOfValue, typename Offsets >
   void
   initValueSize( const Offsets& offsets );

   /**
    * \brief  blocksArray[ i ] stores blocks for sizeof( Value ) == 2^i.
    */
   BlocksType blocksArray[ MaxValueSizeLog() ];

   ViewType view;
};

}  // namespace noa::TNL::Algorithms::SegmentsReductionKernels

#include "CSRAdaptiveKernel.hpp"
