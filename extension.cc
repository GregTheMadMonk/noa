#include <torch/extension.h>

#include <stdexcept>
#include <utility>

#include <functions.hh>

std::size_t tsize(const torch::Tensor& t) {
    std::size_t ret = t.sizes()[0];
    for (std::size_t i = 1; i < t.sizes().size(); ++i) ret *= t.sizes()[i];
    return ret;
}

// DON'T ACTUALLY JUST REINTERPRET_CAST! CHECK DTYPE TOO!
// THIS IS JUST AN EXAMPLE!
std::pair<std::size_t, double*> unpack(torch::Tensor& t) {
    return { tsize(t), reinterpret_cast<double*>(t.data_ptr()) };
}
std::pair<std::size_t, const double*> unpack(const torch::Tensor& t) {
    return { tsize(t), reinterpret_cast<const double*>(t.data_ptr()) };
}

PYBIND11_MODULE(TORCH_EXTENSION_NAME, m) {
    m.def("hi", hi);

    m.def(
        "sum", [] (const torch::Tensor& in) {
            auto [ size, ptr ] = unpack(in);
            return sum(ptr, size);
        }
    );

    m.def(
        "d_sum", [] (const torch::Tensor& in) -> torch::Tensor {
            auto [ size, ptr_in ] = unpack(in);
            auto ret = torch::zeros(size, torch::TensorOptions().dtype(torch::kFloat64));
            auto [ _, ptr_out ] = unpack(ret);
            d_sum(ptr_in, ptr_out, size);
            return ret;
        }
    );

    m.def(
        "dot", [] (const torch::Tensor& a, const torch::Tensor& b) {
            auto [ a_s, a_p ] = unpack(a);
            auto [ b_s, b_p ] = unpack(b);

            if (a_s != b_s) throw std::runtime_error{ "size mismatch" };
            return dot(a_p, b_p, a_s);
        }
    );

    m.def(
        "d_dot", [] (const torch::Tensor& a, const torch::Tensor& b) -> torch::Tensor {
            auto [ a_s, a_p ] = unpack(a);
            auto [ b_s, b_p ] = unpack(b);

            auto ret = torch::zeros({ 2, a_s }, torch::TensorOptions().dtype(torch::kFloat64));
            auto [ _, p_ret ] = unpack(ret);

            if (a_s != b_s) throw std::runtime_error{ "size mismatch" };
            d_dot(a_p, p_ret, b_p, p_ret + a_s, a_s);

            return ret;
        }
    );
}
