// Copyright (c) 2004-2023 Tomáš Oberhuber et al.
//
// This file is part of TNL - Template Numerical Library (https://tnl-project.org/)
//
// SPDX-License-Identifier: MIT

#pragma once

#include <ostream>

#include <noa/3rdparty/tnl-noa/src/TNL/Cuda/CudaCallable.h>

namespace noa::TNL::Algorithms::Segments {

/**
 * \brief Simple structure representing one element of a segment.
 *
 * \tparam Index is type used for indexing of the elements.
 */
template< typename Index >
class SegmentElement
{
public:
   /**
    * \brief Type used for indexing of the elements.
    */
   using IndexType = Index;

   /**
    * \brief Constructor of the segment element with all parameters.
    *
    * \param segmentIdx is in index of the parent segment.
    * \param localIdx is a rank of the element in the segment.
    * \param globalIdx is an index of the element in the related container.
    */
   __cuda_callable__
   SegmentElement( IndexType segmentIdx, IndexType localIdx, IndexType globalIdx )
   : segmentIdx( segmentIdx ), localIdx( localIdx ), globalIdx( globalIdx )
   {}

   /**
    * \brief Returns index of the parent segment.
    *
    * \return index of the parent segment.
    */
   [[nodiscard]] __cuda_callable__
   IndexType
   segmentIndex() const
   {
      return segmentIdx;
   }

   /**
    * \brief Returns rank of the element in the segment.
    *
    * \return rank of the element in the segment.
    */
   [[nodiscard]] __cuda_callable__
   IndexType
   localIndex() const
   {
      return localIdx;
   }

   /**
    * \brief Returns index of the element in the related container.
    *
    * \return index of the element in the related container.
    */
   [[nodiscard]] __cuda_callable__
   IndexType
   globalIndex() const
   {
      return globalIdx;
   }

protected:
   const IndexType segmentIdx;
   const IndexType localIdx;
   const IndexType globalIdx;
};

}  // namespace noa::TNL::Algorithms::Segments
