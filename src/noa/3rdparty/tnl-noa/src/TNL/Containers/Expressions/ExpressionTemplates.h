// Copyright (c) 2004-2023 Tomáš Oberhuber et al.
//
// This file is part of TNL - Template Numerical Library (https://tnl-project.org/)
//
// SPDX-License-Identifier: MIT

#pragma once

#include <ostream>
#include <stdexcept>
#include <utility>

#include <noa/3rdparty/tnl-noa/src/TNL/Functional.h>
#include <noa/3rdparty/tnl-noa/src/TNL/TypeTraits.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Containers/Expressions/TypeTraits.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Containers/Expressions/ExpressionVariableType.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Algorithms/equal.h>
#include <noa/3rdparty/tnl-noa/src/TNL/Algorithms/reduce.h>

namespace noa::TNL {
namespace Containers {
namespace Expressions {

template< typename T1, typename Operation >
struct UnaryExpressionTemplate;

template< typename T1, typename Operation >
struct HasEnabledExpressionTemplates< UnaryExpressionTemplate< T1, Operation > > : std::true_type
{};

template< typename T1,
          typename T2,
          typename Operation,
          ExpressionVariableType T1Type = getExpressionVariableType< T1, T2 >(),
          ExpressionVariableType T2Type = getExpressionVariableType< T2, T1 >() >
struct BinaryExpressionTemplate;

template< typename T1, typename T2, typename Operation, ExpressionVariableType T1Type, ExpressionVariableType T2Type >
struct HasEnabledExpressionTemplates< BinaryExpressionTemplate< T1, T2, Operation, T1Type, T2Type > > : std::true_type
{};

////
// Non-static binary expression template
template< typename T1, typename T2, typename Operation >
struct BinaryExpressionTemplate< T1, T2, Operation, VectorExpressionVariable, VectorExpressionVariable >
{
   using RealType = decltype( Operation{}( std::declval< T1 >()[ 0 ], std::declval< T2 >()[ 0 ] ) );
   using ValueType = RealType;
   using DeviceType = typename T1::DeviceType;
   using IndexType = typename T1::IndexType;
   using ConstViewType = BinaryExpressionTemplate;

   static_assert(
      HasEnabledExpressionTemplates< T1 >::value,
      "Invalid operand in binary expression templates - expression templates are not enabled for the left operand." );
   static_assert(
      HasEnabledExpressionTemplates< T2 >::value,
      "Invalid operand in binary expression templates - expression templates are not enabled for the right operand." );
   static_assert( std::is_same_v< typename T1::DeviceType, typename T2::DeviceType >,
                  "Attempt to mix operands which have different DeviceType." );

   BinaryExpressionTemplate( const T1& a, const T2& b ) : op1( a.getConstView() ), op2( b.getConstView() )
   {
      if( op1.getSize() != op2.getSize() )
         throw std::logic_error( "Attempt to mix operands with different sizes." );
   }

   [[nodiscard]] RealType
   getElement( const IndexType i ) const
   {
      return Operation{}( op1.getElement( i ), op2.getElement( i ) );
   }

   __cuda_callable__
   RealType
   operator[]( const IndexType i ) const
   {
      return Operation{}( op1[ i ], op2[ i ] );
   }

   __cuda_callable__
   RealType
   operator()( const IndexType i ) const
   {
      return operator[]( i );
   }

   [[nodiscard]] __cuda_callable__
   IndexType
   getSize() const
   {
      return op1.getSize();
   }

   [[nodiscard]] ConstViewType
   getConstView() const
   {
      return *this;
   }

protected:
   const typename T1::ConstViewType op1;
   const typename T2::ConstViewType op2;
};

template< typename T1, typename T2, typename Operation >
struct BinaryExpressionTemplate< T1, T2, Operation, VectorExpressionVariable, ArithmeticVariable >
{
   using RealType = decltype( Operation{}( std::declval< T1 >()[ 0 ], std::declval< T2 >() ) );
   using ValueType = RealType;
   using DeviceType = typename T1::DeviceType;
   using IndexType = typename T1::IndexType;
   using ConstViewType = BinaryExpressionTemplate;

   static_assert(
      HasEnabledExpressionTemplates< T1 >::value,
      "Invalid operand in binary expression templates - expression templates are not enabled for the left operand." );

   BinaryExpressionTemplate( const T1& a, const T2& b ) : op1( a.getConstView() ), op2( b ) {}

   [[nodiscard]] RealType
   getElement( const IndexType i ) const
   {
      return Operation{}( op1.getElement( i ), op2 );
   }

   __cuda_callable__
   RealType
   operator[]( const IndexType i ) const
   {
      return Operation{}( op1[ i ], op2 );
   }

   __cuda_callable__
   RealType
   operator()( const IndexType i ) const
   {
      return operator[]( i );
   }

   [[nodiscard]] __cuda_callable__
   IndexType
   getSize() const
   {
      return op1.getSize();
   }

   [[nodiscard]] ConstViewType
   getConstView() const
   {
      return *this;
   }

protected:
   const typename T1::ConstViewType op1;
   const T2 op2;
};

template< typename T1, typename T2, typename Operation >
struct BinaryExpressionTemplate< T1, T2, Operation, ArithmeticVariable, VectorExpressionVariable >
{
   using RealType = decltype( Operation{}( std::declval< T1 >(), std::declval< T2 >()[ 0 ] ) );
   using ValueType = RealType;
   using DeviceType = typename T2::DeviceType;
   using IndexType = typename T2::IndexType;
   using ConstViewType = BinaryExpressionTemplate;

   static_assert(
      HasEnabledExpressionTemplates< T2 >::value,
      "Invalid operand in binary expression templates - expression templates are not enabled for the right operand." );

   BinaryExpressionTemplate( const T1& a, const T2& b ) : op1( a ), op2( b.getConstView() ) {}

   [[nodiscard]] RealType
   getElement( const IndexType i ) const
   {
      return Operation{}( op1, op2.getElement( i ) );
   }

   __cuda_callable__
   RealType
   operator[]( const IndexType i ) const
   {
      return Operation{}( op1, op2[ i ] );
   }

   __cuda_callable__
   RealType
   operator()( const IndexType i ) const
   {
      return operator[]( i );
   }

   [[nodiscard]] __cuda_callable__
   IndexType
   getSize() const
   {
      return op2.getSize();
   }

   [[nodiscard]] ConstViewType
   getConstView() const
   {
      return *this;
   }

protected:
   const T1 op1;
   const typename T2::ConstViewType op2;
};

////
// Non-static unary expression template
template< typename T1, typename Operation >
struct UnaryExpressionTemplate
{
   using RealType = decltype( Operation{}( std::declval< T1 >()[ 0 ] ) );
   using ValueType = RealType;
   using DeviceType = typename T1::DeviceType;
   using IndexType = typename T1::IndexType;
   using ConstViewType = UnaryExpressionTemplate;

   static_assert( HasEnabledExpressionTemplates< T1 >::value,
                  "Invalid operand in unary expression templates - expression templates are not enabled for the operand." );

   // the constructor is explicit to prevent issues with the ternary operator,
   // see https://gitlab.com/tnl-project/tnl/-/issues/140
   explicit UnaryExpressionTemplate( const T1& a ) : operand( a.getConstView() ) {}

   [[nodiscard]] RealType
   getElement( const IndexType i ) const
   {
      return Operation{}( operand.getElement( i ) );
   }

   __cuda_callable__
   RealType
   operator[]( const IndexType i ) const
   {
      return Operation{}( operand[ i ] );
   }

   __cuda_callable__
   RealType
   operator()( const IndexType i ) const
   {
      return operator[]( i );
   }

   [[nodiscard]] __cuda_callable__
   IndexType
   getSize() const
   {
      return operand.getSize();
   }

   [[nodiscard]] ConstViewType
   getConstView() const
   {
      return *this;
   }

protected:
   const typename T1::ConstViewType operand;
};

#ifndef DOXYGEN_ONLY

   #define TNL_MAKE_UNARY_EXPRESSION( fname, functor )                                     \
      template< typename ET1, typename..., EnableIfUnaryExpression_t< ET1, bool > = true > \
      auto fname( const ET1& a )                                                           \
      {                                                                                    \
         return UnaryExpressionTemplate< ET1, functor >( a );                              \
      }

   #define TNL_MAKE_BINARY_EXPRESSION( fname, functor )                                                        \
      template< typename ET1, typename ET2, typename..., EnableIfBinaryExpression_t< ET1, ET2, bool > = true > \
      auto fname( const ET1& a, const ET2& b )                                                                 \
      {                                                                                                        \
         return BinaryExpressionTemplate< ET1, ET2, functor >( a, b );                                         \
      }

TNL_MAKE_BINARY_EXPRESSION( operator+, noa::TNL::Plus )
TNL_MAKE_BINARY_EXPRESSION( operator-, noa::TNL::Minus )
TNL_MAKE_BINARY_EXPRESSION( operator*, noa::TNL::Multiplies )
TNL_MAKE_BINARY_EXPRESSION( operator/, noa::TNL::Divides )
TNL_MAKE_BINARY_EXPRESSION( operator%, noa::TNL::Modulus )
TNL_MAKE_BINARY_EXPRESSION( equalTo, noa::TNL::EqualTo )
TNL_MAKE_BINARY_EXPRESSION( notEqualTo, noa::TNL::NotEqualTo )
TNL_MAKE_BINARY_EXPRESSION( greater, noa::TNL::Greater )
TNL_MAKE_BINARY_EXPRESSION( less, noa::TNL::Less )
TNL_MAKE_BINARY_EXPRESSION( greaterEqual, noa::TNL::GreaterEqual )
TNL_MAKE_BINARY_EXPRESSION( lessEqual, noa::TNL::LessEqual )
// NOTE: noa::TNL::min and noa::TNL::max would conflict with std::min and std::max
//       (For expressions like `min(a, b)` where a and b are TNL Vectors,
//       the ADL consideres even `std::min`, because noa::TNL::Containers::Array
//       has a template parameter `Allocator` that may be `std::allocator`.)
TNL_MAKE_BINARY_EXPRESSION( minimum, noa::TNL::Min )
TNL_MAKE_BINARY_EXPRESSION( maximum, noa::TNL::Max )
TNL_MAKE_BINARY_EXPRESSION( logicalAnd, noa::TNL::LogicalAnd )
TNL_MAKE_BINARY_EXPRESSION( logicalOr, noa::TNL::LogicalOr )
TNL_MAKE_BINARY_EXPRESSION( bitwiseAnd, noa::TNL::BitAnd )
TNL_MAKE_BINARY_EXPRESSION( bitwiseOr, noa::TNL::BitOr )
TNL_MAKE_BINARY_EXPRESSION( bitwiseXor, noa::TNL::BitXor )

TNL_MAKE_UNARY_EXPRESSION( operator+, noa::TNL::UnaryPlus )
TNL_MAKE_UNARY_EXPRESSION( operator-, noa::TNL::UnaryMinus )
TNL_MAKE_UNARY_EXPRESSION( operator!, noa::TNL::LogicalNot )
TNL_MAKE_UNARY_EXPRESSION( operator~, noa::TNL::BitNot )
TNL_MAKE_UNARY_EXPRESSION( abs, noa::TNL::Abs )
TNL_MAKE_UNARY_EXPRESSION( exp, noa::TNL::Exp )
TNL_MAKE_UNARY_EXPRESSION( sqr, noa::TNL::Sqr )
TNL_MAKE_UNARY_EXPRESSION( sqrt, noa::TNL::Sqrt )
TNL_MAKE_UNARY_EXPRESSION( cbrt, noa::TNL::Cbrt )
TNL_MAKE_UNARY_EXPRESSION( log, noa::TNL::Log )
TNL_MAKE_UNARY_EXPRESSION( log10, noa::TNL::Log10 )
TNL_MAKE_UNARY_EXPRESSION( log2, noa::TNL::Log2 )
TNL_MAKE_UNARY_EXPRESSION( sin, noa::TNL::Sin )
TNL_MAKE_UNARY_EXPRESSION( cos, noa::TNL::Cos )
TNL_MAKE_UNARY_EXPRESSION( tan, noa::TNL::Tan )
TNL_MAKE_UNARY_EXPRESSION( asin, noa::TNL::Asin )
TNL_MAKE_UNARY_EXPRESSION( acos, noa::TNL::Acos )
TNL_MAKE_UNARY_EXPRESSION( atan, noa::TNL::Atan )
TNL_MAKE_UNARY_EXPRESSION( sinh, noa::TNL::Sinh )
TNL_MAKE_UNARY_EXPRESSION( cosh, noa::TNL::Cosh )
TNL_MAKE_UNARY_EXPRESSION( tanh, noa::TNL::Tanh )
TNL_MAKE_UNARY_EXPRESSION( asinh, noa::TNL::Asinh )
TNL_MAKE_UNARY_EXPRESSION( acosh, noa::TNL::Acosh )
TNL_MAKE_UNARY_EXPRESSION( atanh, noa::TNL::Atanh )
TNL_MAKE_UNARY_EXPRESSION( floor, noa::TNL::Floor )
TNL_MAKE_UNARY_EXPRESSION( ceil, noa::TNL::Ceil )
TNL_MAKE_UNARY_EXPRESSION( sign, noa::TNL::Sign )

   #undef TNL_MAKE_UNARY_EXPRESSION
   #undef TNL_MAKE_BINARY_EXPRESSION

////
// Pow
template< typename ET1, typename Real, typename..., EnableIfUnaryExpression_t< ET1, bool > = true >
auto
pow( const ET1& a, const Real& exp )
{
   return BinaryExpressionTemplate< ET1, Real, Pow >( a, exp );
}

////
// Cast
template< typename ResultType, typename ET1, typename..., EnableIfUnaryExpression_t< ET1, bool > = true >
auto
cast( const ET1& a )
{
   using CastOperation = typename Cast< ResultType >::Operation;
   return UnaryExpressionTemplate< ET1, CastOperation >( a );
}

////
// Scalar product
template< typename ET1, typename ET2,
          typename..., EnableIfBinaryExpression_t< ET1, ET2, bool > = true >
auto
operator,( const ET1& a, const ET2& b )
{
   return Algorithms::reduce( a * b, noa::TNL::Plus{} );
}

template< typename ET1, typename ET2, typename..., EnableIfBinaryExpression_t< ET1, ET2, bool > = true >
auto
dot( const ET1& a, const ET2& b )
{
   return ( a, b );
}

////
// Vertical operations
template< typename ET1, typename..., EnableIfUnaryExpression_t< ET1, bool > = true >
auto
min( const ET1& a )
{
   return Algorithms::reduce( a, noa::TNL::Min{} );
}

template< typename ET1, typename..., EnableIfUnaryExpression_t< ET1, bool > = true >
auto
argMin( const ET1& a )
{
   return Algorithms::reduceWithArgument( a, noa::TNL::MinWithArg{} );
}

template< typename ET1, typename..., EnableIfUnaryExpression_t< ET1, bool > = true >
auto
max( const ET1& a )
{
   return Algorithms::reduce( a, noa::TNL::Max{} );
}

template< typename ET1, typename..., EnableIfUnaryExpression_t< ET1, bool > = true >
auto
argMax( const ET1& a )
{
   return Algorithms::reduceWithArgument( a, noa::TNL::MaxWithArg{} );
}

template< typename ET1, typename..., EnableIfUnaryExpression_t< ET1, bool > = true >
auto
sum( const ET1& a )
{
   return Algorithms::reduce( a, noa::TNL::Plus{} );
}

template< typename ET1, typename..., EnableIfUnaryExpression_t< ET1, bool > = true >
auto
maxNorm( const ET1& a )
{
   return max( abs( a ) );
}

template< typename ET1, typename..., EnableIfUnaryExpression_t< ET1, bool > = true >
auto
l1Norm( const ET1& a )
{
   return sum( abs( a ) );
}

template< typename ET1, typename..., EnableIfUnaryExpression_t< ET1, bool > = true >
auto
l2Norm( const ET1& a )
{
   using noa::TNL::sqrt;
   return sqrt( sum( sqr( a ) ) );
}

template< typename ET1, typename Real, typename..., EnableIfUnaryExpression_t< ET1, bool > = true >
auto
lpNorm( const ET1& a, const Real& p )
   // since (1.0 / p) has type double, noa::TNL::pow returns double
   -> double
{
   if( p == 1.0 )
      return l1Norm( a );
   if( p == 2.0 )
      return l2Norm( a );
   using noa::TNL::pow;
   return pow( sum( pow( abs( a ), p ) ), 1.0 / p );
}

template< typename ET1, typename..., EnableIfUnaryExpression_t< ET1, bool > = true >
auto
product( const ET1& a )
{
   return Algorithms::reduce( a, noa::TNL::Multiplies{} );
}

template< typename ET1, typename..., EnableIfUnaryExpression_t< ET1, bool > = true >
auto
all( const ET1& a )
{
   return Algorithms::reduce( a, noa::TNL::LogicalAnd{} );
}

template< typename ET1, typename..., EnableIfUnaryExpression_t< ET1, bool > = true >
auto
any( const ET1& a )
{
   return Algorithms::reduce( a, noa::TNL::LogicalOr{} );
}

////
// Comparison operator ==
template< typename ET1, typename ET2, typename..., EnableIfBinaryExpression_t< ET1, ET2, bool > = true >
bool
operator==( const ET1& a, const ET2& b )
{
   // If both operands are vectors, we compare them using array operations.
   // It allows to compare vectors on different devices.
   constexpr bool BothAreNonstaticVectors = IsArrayType< ET1 >::value && IsArrayType< ET2 >::value
                                         && ! IsStaticArrayType< ET1 >::value && ! IsStaticArrayType< ET2 >::value;
   if constexpr( BothAreNonstaticVectors ) {
      if( a.getSize() != b.getSize() )
         return false;
      if( a.getSize() == 0 )
         return true;
      return Algorithms::equal< typename ET1::DeviceType, typename ET2::DeviceType >( a.getData(), b.getData(), a.getSize() );
   }
   else {
      // If some operand is not a vector, we compare them with parallel reduction.
      return all( equalTo( a, b ) );
   }
}

////
// Comparison operator !=
template< typename ET1, typename ET2, typename..., EnableIfBinaryExpression_t< ET1, ET2, bool > = true >
bool
operator!=( const ET1& a, const ET2& b )
{
   return ! operator==( a, b );
}

#endif  // DOXYGEN_ONLY

////
// Output stream
template< typename T1, typename T2, typename Operation >
std::ostream&
operator<<( std::ostream& str, const BinaryExpressionTemplate< T1, T2, Operation >& expression )
{
   str << "[ ";
   for( int i = 0; i < expression.getSize() - 1; i++ )
      str << expression.getElement( i ) << ", ";
   str << expression.getElement( expression.getSize() - 1 ) << " ]";
   return str;
}

template< typename T, typename Operation >
std::ostream&
operator<<( std::ostream& str, const UnaryExpressionTemplate< T, Operation >& expression )
{
   str << "[ ";
   for( int i = 0; i < expression.getSize() - 1; i++ )
      str << expression.getElement( i ) << ", ";
   str << expression.getElement( expression.getSize() - 1 ) << " ]";
   return str;
}

}  // namespace Expressions

// Make all operators visible in the noa::TNL::Containers namespace to be considered
// even for Vector and VectorView
using Expressions::operator!;
using Expressions::operator~;
using Expressions::operator+;
using Expressions::operator-;
using Expressions::operator*;
using Expressions::operator/;
using Expressions::operator%;
using Expressions::operator, ;
using Expressions::operator==;
using Expressions::operator!=;

using Expressions::equalTo;
using Expressions::greater;
using Expressions::greaterEqual;
using Expressions::less;
using Expressions::lessEqual;
using Expressions::notEqualTo;

// Make all functions visible in the noa::TNL::Containers namespace
using Expressions::abs;
using Expressions::acos;
using Expressions::acosh;
using Expressions::all;
using Expressions::any;
using Expressions::argMax;
using Expressions::argMin;
using Expressions::asin;
using Expressions::asinh;
using Expressions::atan;
using Expressions::atanh;
using Expressions::bitwiseAnd;
using Expressions::bitwiseOr;
using Expressions::bitwiseXor;
using Expressions::cast;
using Expressions::cbrt;
using Expressions::ceil;
using Expressions::cos;
using Expressions::cosh;
using Expressions::dot;
using Expressions::exp;
using Expressions::floor;
using Expressions::l1Norm;
using Expressions::l2Norm;
using Expressions::log;
using Expressions::log10;
using Expressions::log2;
using Expressions::logicalAnd;
using Expressions::logicalOr;
using Expressions::lpNorm;
using Expressions::max;
using Expressions::maximum;
using Expressions::maxNorm;
using Expressions::min;
using Expressions::minimum;
using Expressions::pow;
using Expressions::product;
using Expressions::sign;
using Expressions::sin;
using Expressions::sinh;
using Expressions::sqr;
using Expressions::sqrt;
using Expressions::sum;
using Expressions::tan;
using Expressions::tanh;

}  // namespace Containers

// Make all functions visible in the main TNL namespace
using Containers::abs;
using Containers::acos;
using Containers::acosh;
using Containers::all;
using Containers::any;
using Containers::argMax;
using Containers::argMin;
using Containers::asin;
using Containers::asinh;
using Containers::atan;
using Containers::atanh;
using Containers::bitwiseAnd;
using Containers::bitwiseOr;
using Containers::bitwiseXor;
using Containers::cast;
using Containers::cbrt;
using Containers::ceil;
using Containers::cos;
using Containers::cosh;
using Containers::dot;
using Containers::equalTo;
using Containers::exp;
using Containers::floor;
using Containers::greater;
using Containers::greaterEqual;
using Containers::l1Norm;
using Containers::l2Norm;
using Containers::less;
using Containers::lessEqual;
using Containers::log;
using Containers::log10;
using Containers::log2;
using Containers::logicalAnd;
using Containers::logicalOr;
using Containers::lpNorm;
using Containers::max;
using Containers::maximum;
using Containers::maxNorm;
using Containers::min;
using Containers::minimum;
using Containers::notEqualTo;
using Containers::pow;
using Containers::product;
using Containers::sign;
using Containers::sin;
using Containers::sinh;
using Containers::sqr;
using Containers::sqrt;
using Containers::sum;
using Containers::tan;
using Containers::tanh;

////
// Evaluation with reduction
template< typename Vector, typename T1, typename T2, typename Operation, typename Reduction, typename Result >
Result
evaluateAndReduce( Vector& lhs,
                   const Containers::Expressions::BinaryExpressionTemplate< T1, T2, Operation >& expression,
                   const Reduction& reduction,
                   const Result& zero )
{
   using RealType = typename Vector::RealType;
   using IndexType = typename Vector::IndexType;
   using DeviceType = typename Vector::DeviceType;

   RealType* lhs_data = lhs.getData();
   auto fetch = [ = ] __cuda_callable__( IndexType i ) -> RealType
   {
      return ( lhs_data[ i ] = expression[ i ] );
   };
   return Algorithms::reduce< DeviceType >( (IndexType) 0, lhs.getSize(), fetch, reduction, zero );
}

template< typename Vector, typename T1, typename Operation, typename Reduction, typename Result >
Result
evaluateAndReduce( Vector& lhs,
                   const Containers::Expressions::UnaryExpressionTemplate< T1, Operation >& expression,
                   const Reduction& reduction,
                   const Result& zero )
{
   using RealType = typename Vector::RealType;
   using IndexType = typename Vector::IndexType;
   using DeviceType = typename Vector::DeviceType;

   RealType* lhs_data = lhs.getData();
   auto fetch = [ = ] __cuda_callable__( IndexType i ) -> RealType
   {
      return ( lhs_data[ i ] = expression[ i ] );
   };
   return Algorithms::reduce< DeviceType >( (IndexType) 0, lhs.getSize(), fetch, reduction, zero );
}

////
// Addition and reduction
template< typename Vector, typename T1, typename T2, typename Operation, typename Reduction, typename Result >
Result
addAndReduce( Vector& lhs,
              const Containers::Expressions::BinaryExpressionTemplate< T1, T2, Operation >& expression,
              const Reduction& reduction,
              const Result& zero )
{
   using RealType = typename Vector::RealType;
   using IndexType = typename Vector::IndexType;
   using DeviceType = typename Vector::DeviceType;

   RealType* lhs_data = lhs.getData();
   auto fetch = [ = ] __cuda_callable__( IndexType i ) -> RealType
   {
      const RealType aux = expression[ i ];
      lhs_data[ i ] += aux;
      return aux;
   };
   return Algorithms::reduce< DeviceType >( (IndexType) 0, lhs.getSize(), fetch, reduction, zero );
}

template< typename Vector, typename T1, typename Operation, typename Reduction, typename Result >
Result
addAndReduce( Vector& lhs,
              const Containers::Expressions::UnaryExpressionTemplate< T1, Operation >& expression,
              const Reduction& reduction,
              const Result& zero )
{
   using RealType = typename Vector::RealType;
   using IndexType = typename Vector::IndexType;
   using DeviceType = typename Vector::DeviceType;

   RealType* lhs_data = lhs.getData();
   auto fetch = [ = ] __cuda_callable__( IndexType i ) -> RealType
   {
      const RealType aux = expression[ i ];
      lhs_data[ i ] += aux;
      return aux;
   };
   return Algorithms::reduce< DeviceType >( (IndexType) 0, lhs.getSize(), fetch, reduction, zero );
}

////
// Addition and reduction
template< typename Vector, typename T1, typename T2, typename Operation, typename Reduction, typename Result >
Result
addAndReduceAbs( Vector& lhs,
                 const Containers::Expressions::BinaryExpressionTemplate< T1, T2, Operation >& expression,
                 const Reduction& reduction,
                 const Result& zero )
{
   using RealType = typename Vector::RealType;
   using IndexType = typename Vector::IndexType;
   using DeviceType = typename Vector::DeviceType;

   RealType* lhs_data = lhs.getData();
   auto fetch = [ = ] __cuda_callable__( IndexType i ) -> RealType
   {
      const RealType aux = expression[ i ];
      lhs_data[ i ] += aux;
      return noa::TNL::abs( aux );
   };
   return Algorithms::reduce< DeviceType >( (IndexType) 0, lhs.getSize(), fetch, reduction, zero );
}

template< typename Vector, typename T1, typename Operation, typename Reduction, typename Result >
Result
addAndReduceAbs( Vector& lhs,
                 const Containers::Expressions::UnaryExpressionTemplate< T1, Operation >& expression,
                 const Reduction& reduction,
                 const Result& zero )
{
   using RealType = typename Vector::RealType;
   using IndexType = typename Vector::IndexType;
   using DeviceType = typename Vector::DeviceType;

   RealType* lhs_data = lhs.getData();
   auto fetch = [ = ] __cuda_callable__( IndexType i ) -> RealType
   {
      const RealType aux = expression[ i ];
      lhs_data[ i ] += aux;
      return noa::TNL::abs( aux );
   };
   return Algorithms::reduce< DeviceType >( (IndexType) 0, lhs.getSize(), fetch, reduction, zero );
}

}  // namespace noa::TNL

// Helper TNL_ASSERT_ALL_* macros
#include "Assert.h"
