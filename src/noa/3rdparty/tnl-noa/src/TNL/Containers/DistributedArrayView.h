// Copyright (c) 2004-2023 Tomáš Oberhuber et al.
//
// This file is part of TNL - Template Numerical Library (https://tnl-project.org/)
//
// SPDX-License-Identifier: MIT

#pragma once

#include <memory>

#include <noa/3rdparty/tnl-noa/src/TNL/Containers/ArrayView.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Containers/Subrange.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Containers/ByteArraySynchronizer.h>
#include <noa/3rdparty/tnl-noa/src/TNL/MPI/Comm.h>

namespace noa::TNL::Containers {

/**
 * \brief Distributed array view.
 */
template< typename Value, typename Device = Devices::Host, typename Index = int >
class DistributedArrayView
{
public:
   using ValueType = Value;
   using DeviceType = Device;
   using IndexType = Index;
   using LocalRangeType = Subrange< Index >;
   using LocalViewType = Containers::ArrayView< Value, Device, Index >;
   using ConstLocalViewType = Containers::ArrayView< std::add_const_t< Value >, Device, Index >;
   using ViewType = DistributedArrayView< Value, Device, Index >;
   using ConstViewType = DistributedArrayView< std::add_const_t< Value >, Device, Index >;
   using SynchronizerType = ByteArraySynchronizer< DeviceType, IndexType >;

   /**
    * \brief A template which allows to quickly obtain a
    * \ref noa::TNL::Containers::DistributedArrayView "DistributedArrayView" type
    * with changed template parameters.
    */
   template< typename _Value, typename _Device = Device, typename _Index = Index >
   using Self = DistributedArrayView< _Value, _Device, _Index >;

   ~DistributedArrayView();

   // Initialization by raw data
   DistributedArrayView( const LocalRangeType& localRange,
                         IndexType ghosts,
                         IndexType globalSize,
                         MPI::Comm communicator,
                         LocalViewType localData )
   : localRange( localRange ), ghosts( ghosts ), globalSize( globalSize ), communicator( std::move( communicator ) ),
     localData( localData )
   {
      if( localData.getSize() != localRange.getSize() + ghosts )
         throw std::invalid_argument(
            "DistributedArrayView: the local array size does not match the local range of the distributed array" );
      if( ghosts < 0 )
         throw std::invalid_argument( "DistributedArrayView: the ghosts count must be non-negative" );
   }

   DistributedArrayView() = default;

   // Copy-constructor does shallow copy.
   DistributedArrayView( const DistributedArrayView& ) = default;

   // "Templated copy-constructor" accepting any cv-qualification of Value
   template< typename Value_ >
   DistributedArrayView( const DistributedArrayView< Value_, Device, Index >& );

   // default move-constructor
   DistributedArrayView( DistributedArrayView&& ) noexcept = default;

   // method for rebinding (reinitialization) to raw data
   void
   bind( const LocalRangeType& localRange,
         IndexType ghosts,
         IndexType globalSize,
         const MPI::Comm& communicator,
         LocalViewType localData );

   // Note that you can also bind directly to DistributedArray and other types implicitly
   // convertible to DistributedArrayView.
   void
   bind( DistributedArrayView view );

   // binding to local array via raw pointer
   // (local range, ghosts, global size and communicators are preserved)
   template< typename Value_ >
   void
   bind( Value_* data, IndexType localSize );

   [[nodiscard]] const LocalRangeType&
   getLocalRange() const;

   [[nodiscard]] IndexType
   getGhosts() const;

   [[nodiscard]] const MPI::Comm&
   getCommunicator() const;

   [[nodiscard]] LocalViewType
   getLocalView();

   [[nodiscard]] ConstLocalViewType
   getConstLocalView() const;

   [[nodiscard]] LocalViewType
   getLocalViewWithGhosts();

   [[nodiscard]] ConstLocalViewType
   getConstLocalViewWithGhosts() const;

   void
   copyFromGlobal( ConstLocalViewType globalArray );

   // synchronizer stuff
   void
   setSynchronizer( std::shared_ptr< SynchronizerType > synchronizer, int valuesPerElement = 1 );

   [[nodiscard]] std::shared_ptr< SynchronizerType >
   getSynchronizer() const;

   [[nodiscard]] int
   getValuesPerElement() const;

   // Note that this method is not thread-safe - only the thread which created
   // and "owns" the instance of this object can call this method.
   void
   startSynchronization();

   void
   waitForSynchronization() const;

   /*
    * Usual ArrayView methods follow below.
    */

   /**
    * \brief Returns a modifiable view of the array view.
    */
   [[nodiscard]] ViewType
   getView();

   /**
    * \brief Returns a non-modifiable view of the array view.
    */
   [[nodiscard]] ConstViewType
   getConstView() const;

   // Resets the array view to the empty state.
   void
   reset();

   // Returns true if the current array view size is zero.
   [[nodiscard]] bool
   empty() const;

   // TODO: swap

   // Returns the *global* size
   [[nodiscard]] IndexType
   getSize() const;

   // Sets all elements of the array to the given value
   void
   setValue( ValueType value );

   // Safe device-independent element setter
   void
   setElement( IndexType i, ValueType value );

   // Safe device-independent element getter
   [[nodiscard]] ValueType
   getElement( IndexType i ) const;

   // Unsafe element accessor usable only from the Device
   [[nodiscard]] __cuda_callable__
   ValueType&
   operator[]( IndexType i );

   // Unsafe element accessor usable only from the Device
   [[nodiscard]] __cuda_callable__
   const ValueType&
   operator[]( IndexType i ) const;

   // Copy-assignment does deep copy, just like regular array, but the sizes
   // must match (i.e. copy-assignment cannot resize).
   DistributedArrayView&
   operator=( const DistributedArrayView& view );

   // Move-assignment operator
   DistributedArrayView&
   operator=( DistributedArrayView&& ) noexcept = default;

   template< typename Array, typename..., typename = std::enable_if_t< HasSubscriptOperator< Array >::value > >
   DistributedArrayView&
   operator=( const Array& array );

   // Comparison operators
   template< typename Array >
   [[nodiscard]] bool
   operator==( const Array& array ) const;

   template< typename Array >
   [[nodiscard]] bool
   operator!=( const Array& array ) const;

   /**
    * \brief Process the lambda function \e f for each array element in interval [ \e begin, \e end).
    *
    * The lambda function is supposed to be declared as
    *
    * ```
    * f( IndexType elementIdx, ValueType& elementValue )
    * ```
    *
    * where
    *
    * - \e elementIdx is an index of the array element being currently processed
    * - \e elementValue is a value of the array element being currently processed
    *
    * This is performed at the same place where the array is allocated,
    * i.e. it is efficient even on GPU.
    *
    * \param begin The beginning of the array elements interval.
    * \param end The end of the array elements interval.
    * \param f The lambda function to be processed.
    */
   template< typename Function >
   void
   forElements( IndexType begin, IndexType end, Function&& f );

   /**
    * \brief Process the lambda function \e f for each array element in interval [ \e begin, \e end) for constant instances of
    * the array.
    *
    * The lambda function is supposed to be declared as
    *
    * ```
    * f( IndexType elementIdx, const ValueType& elementValue )
    * ```
    *
    * where
    *
    * - \e elementIdx is an index of the array element being currently processed
    * - \e elementValue is a value of the array element being currently processed
    *
    * This is performed at the same place where the array is allocated,
    * i.e. it is efficient even on GPU.
    *
    * \param begin The beginning of the array elements interval.
    * \param end The end of the array elements interval.
    * \param f The lambda function to be processed.
    */
   template< typename Function >
   void
   forElements( IndexType begin, IndexType end, Function&& f ) const;

   void
   loadFromGlobalFile( const String& fileName, bool allowCasting = false );

   void
   loadFromGlobalFile( File& file, bool allowCasting = false );

protected:
   LocalRangeType localRange;
   IndexType ghosts = 0;
   IndexType globalSize = 0;
   MPI::Comm communicator = MPI_COMM_NULL;
   LocalViewType localData;

   std::shared_ptr< SynchronizerType > synchronizer = nullptr;
   int valuesPerElement = 1;
};

}  // namespace noa::TNL::Containers

#include "DistributedArrayView.hpp"
