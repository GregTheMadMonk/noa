// Copyright (c) 2004-2023 Tomáš Oberhuber et al.
//
// This file is part of TNL - Template Numerical Library (https://tnl-project.org/)
//
// SPDX-License-Identifier: MIT

#pragma once

#include <noa/3rdparty/tnl-noa/src/TNL/Algorithms/Segments/CSRView.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Algorithms/Segments/BiEllpackView.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Algorithms/Segments/ChunkedEllpackView.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Algorithms/Segments/EllpackView.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Algorithms/Segments/SlicedEllpackView.h>

#include "CSRScalarKernel.h"
#include "BiEllpackKernel.h"
#include "ChunkedEllpackKernel.h"
#include "EllpackKernel.h"
#include "SlicedEllpackKernel.h"

namespace noa::TNL::Algorithms::SegmentsReductionKernels {

template< typename SegmentsView >
struct DefaultKernel;

template< typename Device, typename Index >
struct DefaultKernel< Segments::CSRView< Device, Index > >
{
   using type = CSRScalarKernel< std::decay_t< Index >, Device >;
};

template< typename Device, typename Index, Segments::ElementsOrganization Organization, int WarpSize >
struct DefaultKernel< Segments::BiEllpackView< Device, Index, Organization, WarpSize > >
{
   using type = BiEllpackKernel< std::decay_t< Index >, Device >;
};

template< typename Device, typename Index, Segments::ElementsOrganization Organization >
struct DefaultKernel< Segments::ChunkedEllpackView< Device, Index, Organization > >
{
   using type = ChunkedEllpackKernel< std::decay_t< Index >, Device >;
};

template< typename Device, typename Index, Segments::ElementsOrganization Organization, int Alignment >
struct DefaultKernel< Segments::EllpackView< Device, Index, Organization, Alignment > >
{
   using type = EllpackKernel< std::decay_t< Index >, Device >;
};

template< typename Device, typename Index, Segments::ElementsOrganization Organization, int SliceSize >
struct DefaultKernel< Segments::SlicedEllpackView< Device, Index, Organization, SliceSize > >
{
   using type = SlicedEllpackKernel< std::decay_t< Index >, Device >;
};

}  // namespace noa::TNL::Algorithms::SegmentsReductionKernels
