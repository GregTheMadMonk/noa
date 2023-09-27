// Copyright (c) 2004-2023 Tomáš Oberhuber et al.
//
// This file is part of TNL - Template Numerical Library (https://tnl-project.org/)
//
// SPDX-License-Identifier: MIT

#pragma once

#include "TridiagonalMatrixView.h"

namespace noa::TNL::Matrices {

template< typename Real, typename Device, typename Index, ElementsOrganization Organization >
__cuda_callable__
TridiagonalMatrixView< Real, Device, Index, Organization >::TridiagonalMatrixView( typename Base::ValuesViewType values,
                                                                                   typename Base::IndexerType indexer )
: Base( std::move( values ), std::move( indexer ) )
{}

template< typename Real, typename Device, typename Index, ElementsOrganization Organization >
__cuda_callable__
void
TridiagonalMatrixView< Real, Device, Index, Organization >::bind( TridiagonalMatrixView view )
{
   Base::bind( std::move( view.values ), std::move( view.indexer ) );
}

template< typename Real, typename Device, typename Index, ElementsOrganization Organization >
auto
TridiagonalMatrixView< Real, Device, Index, Organization >::getView() -> ViewType
{
   return { this->getValues().getView(), this->getIndexer() };
}

template< typename Real, typename Device, typename Index, ElementsOrganization Organization >
auto
TridiagonalMatrixView< Real, Device, Index, Organization >::getConstView() const -> ConstViewType
{
   return { this->getValues().getConstView(), this->getIndexer() };
}

template< typename Real, typename Device, typename Index, ElementsOrganization Organization >
void
TridiagonalMatrixView< Real, Device, Index, Organization >::save( File& file ) const
{
   file.save( &this->rows );
   file.save( &this->columns );
   file << this->values;
}

}  // namespace noa::TNL::Matrices
