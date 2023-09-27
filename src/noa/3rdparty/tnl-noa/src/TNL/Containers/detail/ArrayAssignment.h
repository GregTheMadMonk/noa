// Copyright (c) 2004-2023 Tomáš Oberhuber et al.
//
// This file is part of TNL - Template Numerical Library (https://tnl-project.org/)
//
// SPDX-License-Identifier: MIT

#pragma once

#include <noa/3rdparty/tnl-noa/src/TNL/TypeTraits.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Algorithms/copy.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Algorithms/fill.h>

namespace noa::TNL::Containers::detail {

template< typename Array, typename T, bool isArrayType = IsArrayType< T >::value >
struct ArrayAssignment;

/**
 * \brief Specialization for array-array assignment with containers implementing
 * getArrayData method.
 */
template< typename Array, typename T >
struct ArrayAssignment< Array, T, true >
{
   static void
   resize( Array& a, const T& t )
   {
      a.setSize( t.getSize() );
   }

   static void
   assign( Array& a, const T& t )
   {
      TNL_ASSERT_EQ( a.getSize(), (decltype( a.getSize() )) t.getSize(), "The sizes of the arrays must be equal." );
      Algorithms::copy< typename Array::DeviceType, typename T::DeviceType >( a.getArrayData(), t.getArrayData(), t.getSize() );
   }
};

/**
 * \brief Specialization for array-value assignment for other types. We assume
 * that T is convertible to Array::ValueType.
 */
template< typename Array, typename T >
struct ArrayAssignment< Array, T, false >
{
   static void
   resize( Array& a, const T& t )
   {}

   static void
   assign( Array& a, const T& t )
   {
      Algorithms::fill< typename Array::DeviceType >( a.getArrayData(), (typename Array::ValueType) t, a.getSize() );
   }
};

}  // namespace noa::TNL::Containers::detail
