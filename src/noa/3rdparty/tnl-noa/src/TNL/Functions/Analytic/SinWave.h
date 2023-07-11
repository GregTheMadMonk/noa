// Copyright (c) 2004-2023 Tomáš Oberhuber et al.
//
// This file is part of TNL - Template Numerical Library (https://tnl-project.org/)
//
// SPDX-License-Identifier: MIT

/****
 * Tomas Sobotik
 */
#pragma once

#include <noa/3rdparty/tnl-noa/src/TNL/Config/ParameterContainer.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Containers/StaticVector.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Functions/Domain.h>

namespace noa::TNL::Functions::Analytic {

template< int dimensions, typename Real = double >
class SinWaveBase : public Domain< dimensions, SpaceDomain >
{
public:
   SinWaveBase();

   bool
   setup( const Config::ParameterContainer& parameters, const String& prefix = "" );

   void
   setWaveLength( const Real& waveLength );

   [[nodiscard]] Real
   getWaveLength() const;

   void
   setAmplitude( const Real& amplitude );

   [[nodiscard]] Real
   getAmplitude() const;

   void
   setPhase( const Real& phase );

   [[nodiscard]] Real
   getPhase() const;

   void
   setWavesNumber( const Real& wavesNumber );

   [[nodiscard]] Real
   getWavesNumber() const;

protected:
   [[nodiscard]] bool
   isInsideWaves( const Real& distance ) const;

   Real waveLength, amplitude, phase, wavesNumber;
};

template< int Dimension, typename Real >
class SinWave
{};

template< typename Real >
class SinWave< 1, Real > : public SinWaveBase< 1, Real >
{
public:
   using RealType = Real;
   using PointType = Containers::StaticVector< 1, RealType >;

   template< int XDiffOrder = 0, int YDiffOrder = 0, int ZDiffOrder = 0 >
   [[nodiscard]] __cuda_callable__
   RealType
   getPartialDerivative( const PointType& v, const Real& time = 0.0 ) const;

   [[nodiscard]] __cuda_callable__
   RealType
   operator()( const PointType& v, const Real& time = 0.0 ) const;
};

template< typename Real >
class SinWave< 2, Real > : public SinWaveBase< 2, Real >
{
public:
   using RealType = Real;
   using PointType = Containers::StaticVector< 2, RealType >;

   template< int XDiffOrder = 0, int YDiffOrder = 0, int ZDiffOrder = 0 >
   [[nodiscard]] __cuda_callable__
   RealType
   getPartialDerivative( const PointType& v, const Real& time = 0.0 ) const;

   [[nodiscard]] __cuda_callable__
   RealType
   operator()( const PointType& v, const Real& time = 0.0 ) const;
};

template< typename Real >
class SinWave< 3, Real > : public SinWaveBase< 3, Real >
{
public:
   using RealType = Real;
   using PointType = Containers::StaticVector< 3, RealType >;

   template< int XDiffOrder = 0, int YDiffOrder = 0, int ZDiffOrder = 0 >
   [[nodiscard]] __cuda_callable__
   RealType
   getPartialDerivative( const PointType& v, const Real& time = 0.0 ) const;

   [[nodiscard]] __cuda_callable__
   RealType
   operator()( const PointType& v, const Real& time = 0.0 ) const;
};

template< int Dimension, typename Real >
std::ostream&
operator<<( std::ostream& str, const SinWave< Dimension, Real >& f )
{
   str << "Sin Wave. function: amplitude = " << f.getAmplitude() << " wavelength = " << f.getWaveLength()
       << " phase = " << f.getPhase() << " waves number = " << f.getWavesNumber();
   return str;
}

}  // namespace noa::TNL::Functions::Analytic

#include <noa/3rdparty/tnl-noa/src/TNL/Functions/Analytic/SinWave_impl.h>
