// Standard library
#include <concepts>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

// Torch/pybind
#include <torch/extension.h>
#include <pybind11/embed.h>
#include <pybind11/functional.h>

// NOA headers
#include <noa/bindings/python/dtype.hh>
#include <noa/bindings/python/wrapper.hh>
#include <noa/cfd/cfd.hh>
#include <noa/utils/combine/combine.hh>
#include <noa/utils/common/meta.hh>

namespace cmb = noa::utils::combine;
namespace cfd = noa::cfd;

#define NOA_STR_INT(x) #x
#define NOA_STR(x) NOA_STR_INT(x)

template <typename Real>
using Domain = noa::utils::domain::Domain<
    noa::TNL::Meshes::Topologies::Triangle,
    noa::TNL::Devices::Host,
    Real
>;
template <typename Real>
using Problem = cfd::CFDProblem<Domain<Real>>;

template <typename Real>
using Composer = cmb::DynamicComposer<
    cfd::FinDiff<cfd::MHFE<Domain<Real>, false>>,
    cfd::FinDiff<cfd::MHFE<Domain<Real>, true>>,
    cfd::ForwardDiff<Domain<Real>>
>;

template <typename Point>
constexpr auto wrapPoint(Point p) {
    return torch::tensor({ p[0], p[1] });
}

void hi() {
    std::cout << "Hello from noa::cfd!" << std::endl;
}

template <typename DomainType, typename F, std::size_t dim = 0>
requires (dim <= DomainType::dCell)
decltype(auto) visitDimension(
    std::size_t dimension,
    const typename DomainType::MeshType& mesh,
    F&& f
) {
    if (dim == dimension) {
        return f(noa::utils::meta::ValTag<dim>{}, mesh);
    } else if constexpr (dim < DomainType::dCell) {
        return
            visitDimension<DomainType, F, dim + 1>(
                dimension, mesh, std::forward<F>(f)
            );
    }
}

template <typename Real>
void wrapDomain(auto& m, std::string_view name) {
    namespace pynoa = noa::bindings::python;
    const auto noGil = py::call_guard<py::gil_scoped_release>();

    using DomainType = Domain<Real>;
    using Wrapper =
        pynoa::WeakWrapper<DomainType>;
    using Point = typename Domain<Real>::Point;
    using Edge = typename Domain<Real>::Edge;
    py::class_<Point>(m, (std::string{name.data()} + "Point").c_str())
        .def(
            "coords",
            [] (Point p) {
                return wrapPoint(p.point());
            }, noGil
        )
        ;
    py::class_<Edge>(m, (std::string{name.data()} + "Edge").c_str())
        .def(
            "points",
            [] (const Edge& e) {
                std::vector<torch::Tensor> ret{};
                for (const auto& p : e.points()) {
                    ret.push_back(wrapPoint(p));
                }
                return ret;
            } , noGil
        )
        .def(
            "center", 
            [] (const Edge& e) {
                return wrapPoint(e.center());
            }, noGil
        )
        .def(
            "normal",
            [] (const Edge& e) {
                return wrapPoint(e.normal());
            }, noGil
        )
        ;
    py::class_<Wrapper>(m, name.data())
        .def(
            "getEntitiesCount",
            [] (Wrapper domain, std::size_t dim) -> std::size_t {
                return visitDimension<DomainType>(
                    dim, domain->getMesh(),
                    [domain] <std::size_t d>
                    (noa::utils::meta::ValTag<d>, const auto& mesh) {
                        return domain->getMesh()
                            .template getEntitiesCount<d>();
                    }
                );
            },
            noGil
        )
        .def(
            "getEdge", [] (Wrapper domain, std::size_t num) {
                return domain->getEdge(num);
            }, noGil
        )
        .def(
            "getPoint", [] (Wrapper domain, std::size_t num) {
                return domain->getPoint(num);
            }, noGil
        )
        .def(
            "forBoundaryEdges",
            [] (Wrapper domain, const std::function<void(std::size_t, Edge)>& f) {
                domain->getMesh().template forBoundary<DomainType::dEdge>(
                    [&f, domain] (auto edge) {
                        f(edge, domain->getEdge(edge));
                    }
                );
            }, noGil
        )
        .def(
            "getLayers",
            [] (Wrapper domain, std::size_t dim) {
                std::vector<std::string> ret{};

                for (const auto& [ i, layer ] : domain->getLayers(dim)) {
                    if (!layer.exportHint || layer.alias.empty()) {
                        continue;
                    }
                    ret.push_back(layer.alias);
                }

                return ret;
            },
            noGil
        )
        .def(
            "getLayer",
            [] (Wrapper domain, std::size_t dim, const std::string& name) -> torch::Tensor {
                auto& lrs = domain->getLayers(dim);

                const auto it = std::find_if(
                    lrs.begin(), lrs.end(),
                    [name] (const auto& p) {
                        return p.second.alias == name;
                    }
                );

                if (it == lrs.end()) {
                    throw std::runtime_error{
                        std::string{"No layer "} + name.data()
                    };
                }

                return it->second.visit(
                    [] (auto& vec) -> torch::Tensor {
                        const auto size = vec.getSize();
                        using Value =
                            std::remove_cvref_t<decltype(vec[0])>;

                        const auto opt =
                            torch::TensorOptions()
                            .dtype(pynoa::dtype<Value>);
                        return torch::from_blob(vec.getData(), size, opt);
                    }
                );
            },
            noGil
        )
        .def(
            "generateGrid",
            [] (
                Wrapper domain,
                std::array<typename DomainType::GlobalIndexType, 2> N,
                std::array<Real, 2> d
            ) {
                domain->generateGrid(N, d);
            },
            noGil
        )
        .def(
            "generateGrid",
            [] (
                Wrapper domain,
                std::array<typename DomainType::GlobalIndexType, 2> N,
                std::array<Real, 2> d,
                std::array<Real, 2> offset
            ) {
                domain->generateGrid(N, d, offset);
            },
            noGil
        )
        .def(
            "write",
            [] (Wrapper domain, const std::string& path) {
                domain->write(path);
            }, noGil
        )
        .def(
            "vistify",
            [] (Wrapper domain) {
                // Import python modules
                const auto pyvista = py::module_::import("pyvista");

                // Save domain to the temp file
                domain->write("/tmp/tmpdomain.vtu");
                // Load the file with pyvista
                return pyvista.attr("read")("/tmp/tmpdomain.vtu");
            }
        )
        ;
} // <-- void wrapDomain(m, name)

template <typename Real>
struct ComposerWrapper {
    Composer<Real> comp{};
}; // <-- struct ComposerWrapper

template <typename Real>
void wrapComposer(auto& m, std::string_view name) {
    namespace pynoa = noa::bindings::python;
    const auto noGil = py::call_guard<py::gil_scoped_release>();

    py::class_<ComposerWrapper<Real>>(m, name.data())
        .def(py::init<>())
        .def(
            "getDomain",
            [] (ComposerWrapper<Real>& comp)
            -> std::optional<pynoa::WeakWrapper<Domain<Real>>> {
                try {
                    return pynoa::WeakWrapper{
                        comp.comp.template get<Problem<Real>>()
                            .getDomainForChange()
                    };
                } catch (const std::out_of_range& e) {
                    return std::nullopt;
                }
            },
            noGil
        )
        .def(
            "setTasks",
            [] (
                ComposerWrapper<Real>& composer,
                const std::vector<std::string>& tasks,
                const std::function<
                    void(pynoa::WeakWrapper<Domain<Real>>)
                >& setup
            ) {
                composer.comp.setTasks(
                    tasks, {
                        [&setup] (Problem<Real>& prob) {
                            setup(prob.getDomainForChange());
                        }
                    }
                );
            },
            noGil
        )
        .def(
            "run",
            [] (ComposerWrapper<Real>& composer) { composer.comp.run(); },
            noGil
        )
        .def(
            "listTasks",
            [] () { return Composer<Real>::getAllowedTasks(); },
            noGil
        )
        .def(
            "listTasks",
            [] (ComposerWrapper<Real>&) {
                return Composer<Real>::getAllowedTasks();
            },
            noGil
        )
        ;
} // <-- void wrapComposer(m, name)

PYBIND11_MODULE(TORCH_EXTENSION_NAME, m) {
    namespace pynoa = noa::bindings::python;
    const auto noGil = py::call_guard<py::gil_scoped_release>();

    wrapDomain<float>(m, "f32_Domain_ref");
    wrapDomain<double>(m, "f64_Domain_ref");
    wrapComposer<float>(m, "f32");
    wrapComposer<double>(m, "f64");

    m.def("hi", &hi, noGil, "Say hi");
}
