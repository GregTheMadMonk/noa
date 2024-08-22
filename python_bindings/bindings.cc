// #include <torch/torch.h>
// #include <torch/python.h>
#include <pybind11/detail/common.h> // Pybind11 macros

import example_module;
import torchext;
import std;

namespace em = example_module;

PYBIND11_MODULE(python_bindings, m) {
    m.def("hi", [] { std::println("Hi"); });

    m.def(
        "sum", [] (const torch::Tensor& t) {
            double ret;
            torchext::visit(
                [&ret] <typename T> (std::span<const T> data) {
                    ret = em::sum_span(data);
                }, t
            );
            return ret;
        }
    );

    m.def(
        "sum_diff", [] (const torch::Tensor& in, torch::Tensor& out) {
            assert(in.dtype() == out.dtype());
            // Move away from Torch API ASAP because it is built with
            // GNU libstdc++ and we use libc++ (shoutout to undefined symbol
            // errors gotta be one of my favorite errors)
            torchext::visit( // TODO: Muti-tensor visit
                [&out] <typename T> (std::span<const T> s_in) {
                    torchext::visit(
                        [s_in] (std::span<T> s_out) {
                            assert(s_in.size() == s_out.size());
                            em::sum_span_diff(s_in, s_out);
                        }, out
                    );
                }, in
            );
            return out;
        }
    );
}
