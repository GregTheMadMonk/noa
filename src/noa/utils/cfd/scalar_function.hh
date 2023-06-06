/**
 * \file scalar_function.hh
 * \brief Defines the concept for CFD solution sclar function
 */

#pragma once

#include <noa/utils/domain/domain.hh>

namespace noa::utils::cfd {

/// \brief Concept for a solution scalar function
///
/// Scalar function must take a TNL vector view to the problem solution and
/// return a corresponding real scalar
template <typename ScalarFunctionType, typename DomainType>
concept CScalarFunc = requires {
    requires domain::CDomain<DomainType>;
    requires std::same_as<
        std::invoke_result_t<
            ScalarFunctionType,
            typename DomainType::LayerManagerType::Vector<typename DomainType::RealType>::ConstViewType
        >,
        typename DomainType::RealType
    >;
}; // <-- concept CScalarFunc

/// \brief Concept for a solution scalr function partial derivative wrt problem solution
///
/// Accepts two arguments: first being a solution vector view, and the second one
/// representing the vector view where the derivative should be stored
template <typename ScalarFunctionDerivativeType, typename DomainType>
concept CScalarFuncWrtP = requires {
    requires domain::CDomain<DomainType>;
    requires std::invocable<
        ScalarFunctionDerivativeType,
        typename DomainType::LayerManagerType::Vector<typename DomainType::RealType>::ConstViewType,
        typename DomainType::LayerManagerType::Vector<typename DomainType::RealType>::ViewType
    >;
}; // <-- concept CScalarFuncWrtP

} // <-- namespace noa::utils::cfd
