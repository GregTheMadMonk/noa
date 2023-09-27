/**
 * @file torch_dtype.hh
 * @brief Conversion of std type to torch dtypes
 */
#pragma once

// Standard library
#include <concepts>
#include <cstdint>

// Torch
#include <torch/torch.h>

namespace noa::bindings::python {

namespace detail {

    template <std::size_t sz> requires (sz > 0) struct TDTReal;

    template <>
    struct TDTReal<2> { static constexpr auto value = torch::kFloat16; };
    template <>
    struct TDTReal<4> { static constexpr auto value = torch::kFloat32; };
    template <>
    struct TDTReal<8> { static constexpr auto value = torch::kFloat64; };

} // <-- namespace detail

/// @brief Get torch's floating point dtype
template <typename T>
constexpr auto real = detail::TDTReal<sizeof(T)>::value;

namespace detail {
    template <std::size_t sz> requires (sz > 0) struct TDTInt;

    template <>
    struct TDTInt<1> { static constexpr auto value = torch::kInt8; };
    template <>
    struct TDTInt<2> { static constexpr auto value = torch::kInt16; };
    template <>
    struct TDTInt<4> { static constexpr auto value = torch::kInt32; };
    template <>
    struct TDTInt<8> { static constexpr auto value = torch::kInt64; };
} // <-- namespace detail

/// @vrief Convert std type to Torch's integer dtype
template <typename T>
constexpr auto integer = detail::TDTInt<sizeof(T)>::value;

namespace detail {
    template <typename T> struct TDType;

    template <std::integral T> struct TDType<T> {
        static constexpr auto value = integer<T>;
    };

    template <std::floating_point T> struct TDType<T> {
        static constexpr auto value = real<T>;
    };
}

/// @brief Convert some std type to Torch's dtype
template <typename T>
constexpr auto dtype = detail::TDType<T>::value;

} // <-- namespace noa::bindings::python
