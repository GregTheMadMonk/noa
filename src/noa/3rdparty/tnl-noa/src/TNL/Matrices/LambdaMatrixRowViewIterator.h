// Copyright (c) 2004-2023 Tomáš Oberhuber et al.
//
// This file is part of TNL - Template Numerical Library (https://tnl-project.org/)
//
// SPDX-License-Identifier: MIT

#pragma once

#include <ostream>

#include <noa/3rdparty/tnl-noa/src/TNL/Cuda/CudaCallable.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Matrices/LambdaMatrixElement.h>

namespace noa::TNL::Matrices {

template< typename RowView >
class LambdaMatrixRowViewIterator
{
public:
   /**
    * \brief Type of LambdaMatrixRowView
    */
   using RowViewType = RowView;

   /**
    * \brief The type of matrix elements.
    */
   using RealType = typename RowViewType::RealType;

   /**
    * \brief The type used for matrix elements indexing.
    */
   using IndexType = typename RowViewType::IndexType;

   /**
    * \brief The type of related matrix element.
    */
   using MatrixElementType = typename RowView::MatrixElementType;

   /**
    * \brief Tells whether the parent matrix is a binary matrix.
    * @return `true` if the matrix is binary.
    */
   [[nodiscard]] static constexpr bool
   isBinary()
   {
      return RowViewType::isBinary();
   }

   __cuda_callable__
   LambdaMatrixRowViewIterator( const RowViewType& rowView, const IndexType& localIdx );

   /**
    * \brief Comparison of two matrix row iterators.
    *
    * \param other is another matrix row iterator.
    * \return \e true if both iterators points at the same point of the same matrix, \e false otherwise.
    */
   [[nodiscard]] __cuda_callable__
   bool
   operator==( const LambdaMatrixRowViewIterator& other ) const;

   /**
    * \brief Comparison of two matrix row iterators.
    *
    * \param other is another matrix row iterator.
    * \return \e false if both iterators points at the same point of the same matrix, \e true otherwise.
    */
   [[nodiscard]] __cuda_callable__
   bool
   operator!=( const LambdaMatrixRowViewIterator& other ) const;

   __cuda_callable__
   LambdaMatrixRowViewIterator&
   operator++();

   __cuda_callable__
   LambdaMatrixRowViewIterator&
   operator--();

   [[nodiscard]] __cuda_callable__
   MatrixElementType
   operator*();

   [[nodiscard]] __cuda_callable__
   MatrixElementType
   operator*() const;

protected:
   const RowViewType& rowView;

   IndexType localIdx = 0;
};

}  // namespace noa::TNL::Matrices

#include <noa/3rdparty/tnl-noa/src/TNL/Matrices/LambdaMatrixRowViewIterator.hpp>
