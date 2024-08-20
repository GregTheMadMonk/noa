#include <torch/torch.h>
#include <torch/python.h>

#include <cassert>
#include <enzyme/enzyme>

import std;

namespace detail {
    template <typename T>
    inline constexpr
    T sum_ptr(const T* v, std::size_t size) {
        T ret = 0;
        for (std::size_t i = 0; i < size; ++i) ret += v[i];
        return ret;
    }
}

template <typename T>
inline constexpr
T sum_span(std::span<const T> v)
{ return detail::sum_ptr(v.data(), v.size()); }

template <typename T>
inline
void sum_span_diff(std::span<const T> v, std::span<T> derivative) {
    assert(v.size() == derivative.size());
    enzyme::autodiff<enzyme::Reverse>(
        detail::sum_ptr<T>,
        enzyme::Duplicated<const T*>{ v.data(), derivative.data() },
        enzyme::Const{ v.size() }
    );
}

inline constexpr
std::size_t tensor_size(const torch::Tensor& t) {
    std::size_t ret = t.sizes()[0];
    for (std::size_t i = 1; i < t.sizes().size(); ++i) {
        ret *= t.sizes()[i];
    }
    return ret;
}

PYBIND11_MODULE(example_module, m) {
    m.def(
        "sum", [] (const torch::Tensor& t) {
            return sum_span(
                std::span{
                    reinterpret_cast<const double*>(t.data_ptr()),
                    tensor_size(t)
                }
            );
        }
    );

    m.def(
        "sum_diff", [] (const torch::Tensor& t) {
            const auto size = tensor_size(t);
            auto ret = torch::zeros_like(t);
            sum_span_diff(
                std::span{ reinterpret_cast<const double*>(t.data_ptr()), size },
                std::span{ reinterpret_cast<double*>(ret.data_ptr()),     size }
            );
            return ret;
        }
    );
}
