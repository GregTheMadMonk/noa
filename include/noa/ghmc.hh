/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2021, Roland Grinis, GrinisRIT ltd. (roland.grinis@grinisrit.com)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "noa/utils/numerics.hh"

#include <iostream>
#include <chrono>

#include <torch/torch.h>

namespace noa::ghmc {

    using Parameters = utils::Tensors;
    using Momentum = utils::Tensors;
    using MomentumOpt = std::optional<Momentum>;
    using LogProbability = utils::Tensor;
    using LogProbabilityGraph = utils::ADGraph;
    using Spectrum = utils::Tensors;
    using Rotation = utils::Tensors;
    using MetricDecomposition = std::tuple<Spectrum, Rotation>;
    using MetricDecompositionOpt = std::optional<MetricDecomposition>;
    using Energy = utils::Tensor;
    using PhaseSpaceFoliation = std::tuple<Parameters, Momentum, Energy>;
    using PhaseSpaceFoliationOpt = std::optional<PhaseSpaceFoliation>;

    using ParametersFlow = std::vector<Parameters>;
    using MomentumFlow = std::vector<Momentum>;
    using EnergyLevel = std::vector<Energy>;

    using HamiltonianFlow = std::tuple<ParametersFlow, MomentumFlow, EnergyLevel>;
    using ParametersGradient = utils::Tensors;
    using MomentumGradient = utils::Tensors;
    using HamiltonianGradient = std::tuple<ParametersGradient, MomentumGradient>;
    using HamiltonianGradientOpt = std::optional<HamiltonianGradient>;
    using Samples = std::vector<Parameters>;



    template<typename Dtype>
    struct Configuration {
        uint32_t max_flow_steps = 3;
        Dtype step_size = 0.1f;
        Dtype binding_const = 100.f;
        Dtype cutoff = 1e-6f;
        Dtype jitter = 1e-6f;
        Dtype softabs_const = 1e6f;
        bool verbose = false;

        inline Configuration &set_max_flow_steps(const Dtype &max_flow_steps_) {
            max_flow_steps = max_flow_steps_;
            return *this;
        }

        inline Configuration &set_step_size(const Dtype &step_size_) {
            step_size = step_size_;
            return *this;
        }

        inline Configuration &set_binding_const(const Dtype &binding_const_) {
            binding_const = binding_const_;
            return *this;
        }

        inline Configuration &set_cutoff(const Dtype &cutoff_) {
            cutoff = cutoff_;
            return *this;
        }

        inline Configuration &set_jitter(const Dtype &jitter_) {
            jitter = jitter_;
            return *this;
        }

        inline Configuration &set_softabs_const(const Dtype &softabs_const_) {
            softabs_const = softabs_const_;
            return *this;
        }

        inline Configuration &set_verbosity(bool verbose_) {
            verbose = verbose_;
            return *this;
        }
    };

    template<typename Configurations>
    inline auto softabs_metric(const Configurations &conf) {
        return [conf](const LogProbabilityGraph &log_prob_graph) {
            const auto hess_ = utils::numerics::hessian(log_prob_graph);
            if (!hess_.has_value()) {
                if (conf.verbose)
                    std::cerr << "GHMC: failed to compute hessian for log probability\n"
                              << std::get<LogProbability>(log_prob_graph) << "\n";
                return MetricDecompositionOpt{};
            }

            const auto nparam = hess_.value().size();
            auto spectrum = Spectrum{};
            spectrum.reserve(nparam);
            auto rotation = Rotation{};
            rotation.reserve(nparam);

            for (const auto &hess : hess_.value()) {
                const auto n = hess.size(0);
                const auto[eigs, Q] = torch::symeig(
                        -hess + conf.jitter * torch::eye(n, hess.options()) * torch::rand(n, hess.options()),
                        true);

                const utils::Tensor check_Q = Q.detach().sum();
                if (torch::isnan(check_Q).item<bool>() || torch::isinf(check_Q).item<bool>()) {
                    std::cerr << "GHMC: failed to compute local rotation matrix for log probability\n"
                              << std::get<LogProbability>(log_prob_graph) << "\n";
                    return MetricDecompositionOpt{};
                }

                const auto reg_eigs = torch::where(eigs.abs() >= conf.cutoff, eigs,
                                                   torch::tensor(conf.cutoff, hess.options()));
                const auto softabs = torch::abs((1 / torch::tanh(conf.softabs_const * reg_eigs)) * reg_eigs);

                const utils::Tensor check_softabs = softabs.detach().sum();
                if (torch::isnan(check_softabs).item<bool>() || torch::isinf(check_softabs).item<bool>()) {
                    std::cerr << "GHMC: failed to compute SoftAbs map for log probability\n"
                              << std::get<LogProbability>(log_prob_graph) << "\n";
                    return MetricDecompositionOpt{};
                }

                spectrum.push_back(softabs);
                rotation.push_back(Q);
            }
            return MetricDecompositionOpt{MetricDecomposition{spectrum, rotation}};
        };
    }

    template<typename LogProbabilityDensity, typename Configurations>
    inline auto hamiltonian(const LogProbabilityDensity &log_prob_density, const Configurations &conf) {
        const auto local_metric = softabs_metric(conf);
        return [log_prob_density, local_metric, conf](
                const Parameters &parameters,
                const MomentumOpt &momentum_ = std::nullopt) {
            const auto log_prob_graph = log_prob_density(parameters);
            const LogProbability check_log_prob = std::get<LogProbability>(log_prob_graph).detach();
            if (torch::isnan(check_log_prob).item<bool>() || torch::isinf(check_log_prob).item<bool>()) {
                if (conf.verbose)
                    std::cerr << "GHMC: failed to compute log probability.\n";
                return PhaseSpaceFoliationOpt{};
            }

            const auto metric = local_metric(log_prob_graph);
            if (!metric.has_value()) {
                if (conf.verbose)
                    std::cerr << "GHMC: failed to compute local metric for log probability\n"
                              << std::get<LogProbability>(log_prob_graph) << "\n";
                return PhaseSpaceFoliationOpt{};
            }
            const auto&[spectrum, rotation] = metric.value();

            auto energy = -std::get<LogProbability>(log_prob_graph);

            const auto nparam = parameters.size();
            auto momentum = Momentum{};
            momentum.reserve(nparam);

            for (uint32_t i = 0; i < nparam; i++) {

                const auto &rotation_i = rotation.at(i);
                const auto &spectrum_i = spectrum.at(i);

                const auto momentum_lift = momentum_.has_value()
                                           ? momentum_.value().at(i)
                                           : rotation_i.detach().mv(
                                torch::sqrt(spectrum_i.detach()) * torch::randn_like(spectrum_i));

                const auto momentum_i = momentum_lift.detach().view_as(parameters.at(i)).requires_grad_(true);

                const auto first_order_term = spectrum_i.log().sum() / 2;
                const auto mass = rotation_i.mm(torch::diag(1 / spectrum_i)).mm(rotation_i.t());

                const auto momentum_vec = momentum_i.flatten();
                const auto second_order_term = momentum_vec.dot(mass.mv(momentum_vec)) / 2;

                energy += first_order_term + second_order_term;
                momentum.push_back(momentum_i);
            }

            const Energy check_energy = energy.detach();
            if (torch::isnan(check_energy).item<bool>() || torch::isinf(check_energy).item<bool>()) {
                if (conf.verbose)
                    std::cerr << "GHMC: failed to compute Hamiltonian for log probability\n"
                              << std::get<LogProbability>(log_prob_graph) << "\n";
                return PhaseSpaceFoliationOpt{};
            }

            return PhaseSpaceFoliationOpt{
                    PhaseSpaceFoliation{std::get<Parameters>(log_prob_graph), momentum, energy}};
        };
    }

    template<typename Configurations>
    inline auto hamiltonian_gradient(const Configurations &conf) {
        return [conf](const PhaseSpaceFoliationOpt &foliation) {
            if (!foliation.has_value()) {
                if (conf.verbose)
                    std::cerr << "GHMC: no phase space foliation provided.\n";
                return HamiltonianGradientOpt{};
            }
            const auto &[params, momentum, energy] = foliation.value();

            const auto nparam = params.size();
            auto variables = utils::Tensors{};
            variables.reserve(2 * nparam);

            variables.insert(variables.end(), params.begin(), params.end());
            variables.insert(variables.end(), momentum.begin(), momentum.end());

            const auto ham_grad = torch::autograd::grad({energy}, variables);

            auto params_grad = ParametersGradient{};
            params_grad.reserve(nparam);

            for (uint32_t i = 0; i < nparam; i++) {
                const auto params_grad_i = ham_grad.at(i).detach();
                const auto check_params = params_grad_i.sum();
                if (torch::isnan(check_params).item<bool>() || torch::isinf(check_params).item<bool>()) {
                    if (conf.verbose)
                        std::cerr << "GHMC: failed to compute parameters gradient for Hamiltonian\n"
                                  << energy << "\n";
                    return HamiltonianGradientOpt{};
                } else params_grad.push_back(params_grad_i);
            }

            auto momentum_grad = MomentumGradient{};
            momentum_grad.reserve(nparam);

            for (uint32_t i = nparam; i < 2 * nparam; i++) {
                const auto momentum_grad_i = ham_grad.at(i).detach();
                const auto check_momentum = momentum_grad_i.sum();
                if (torch::isnan(check_momentum).item<bool>() || torch::isinf(check_momentum).item<bool>()) {
                    if (conf.verbose)
                        std::cerr << "GHMC: failed to compute momentum gradient for Hamiltonian\n"
                                  << energy << "\n";
                    return HamiltonianGradientOpt{};
                } else momentum_grad.push_back(momentum_grad_i);
            }

            return HamiltonianGradientOpt{HamiltonianGradient{params_grad, momentum_grad}};
        };
    }


    template<typename LogProbabilityDensity, typename Configurations>
    inline auto hamiltonian_flow(const LogProbabilityDensity &log_prob_density, const Configurations &conf) {
        const auto ham = hamiltonian(log_prob_density, conf);
        const auto ham_grad = hamiltonian_gradient(conf);
        const auto theta = 2 * conf.binding_const * conf.step_size;
        const auto rot = std::make_tuple(cos(theta), sin(theta));
        return [ham, ham_grad, conf, rot](const Parameters &parameters,
                                          const MomentumOpt &momentum_ = std::nullopt) {
            auto params_flow = ParametersFlow{};
            params_flow.reserve(conf.max_flow_steps + 1);

            auto momentum_flow = MomentumFlow{};
            momentum_flow.reserve(conf.max_flow_steps + 1);

            auto energy_level = EnergyLevel{};
            energy_level.reserve(conf.max_flow_steps + 1);

            auto foliation = ham(parameters, momentum_);
            if (!foliation.has_value()) {
                if (conf.verbose)
                    std::cerr << "GHMC: failed to initialise Hamiltonian flow.\n";
                return HamiltonianFlow{params_flow, momentum_flow, energy_level};
            }

            const auto &[initial_params, initial_momentum, initial_energy] = foliation.value();

            const auto nparam = parameters.size();
            auto params = Parameters{};
            params.reserve(nparam);
            auto momentum_copy = Momentum{};
            momentum_copy.reserve(nparam);

            for (uint32_t i = 0; i < nparam; i++) {
                params.push_back(initial_params.at(i).detach());
                momentum_copy.push_back(initial_momentum.at(i).detach());
            }

            params_flow.push_back(params);
            momentum_flow.push_back(momentum_copy);
            energy_level.push_back(initial_energy.detach());

            uint32_t iter_step = 0;
            if (iter_step >= conf.max_flow_steps)
                return HamiltonianFlow{params_flow, momentum_flow, energy_level};

            const auto error_msg = [&iter_step, &conf]() {
                if (conf.verbose)
                    std::cerr << "GHMC: failed to evolve flow at step "
                              << iter_step + 1 << "/" << conf.max_flow_steps << "\n";
            };

            auto dynamics = ham_grad(foliation);
            if (!dynamics.has_value()) {
                error_msg();
                return HamiltonianFlow{params_flow, momentum_flow, energy_level};
            }

            const auto delta = conf.step_size / 2;
            const auto &[c, s] = rot;

            auto params_copy = params;
            auto momentum = momentum_copy;

            for (uint32_t i = 0; i < nparam; i++) {
                params_copy.at(i) = params_copy.at(i) + std::get<1>(dynamics.value()).at(i) * delta;
                momentum.at(i) = momentum.at(i) - std::get<0>(dynamics.value()).at(i) * delta;
            }

            for (iter_step = 0; iter_step < conf.max_flow_steps; iter_step++) {

                foliation = ham(params_copy, momentum);
                dynamics = ham_grad(foliation);
                if (!dynamics.has_value()) {
                    error_msg();
                    break;
                }

                for (uint32_t i = 0; i < nparam; i++) {

                    params.at(i) = params.at(i) + std::get<1>(dynamics.value()).at(i) * delta;
                    momentum_copy.at(i) = momentum_copy.at(i) - std::get<0>(dynamics.value()).at(i) * delta;

                    params.at(i) = (params.at(i) + params_copy.at(i) +
                                    c * (params.at(i) - params_copy.at(i)) +
                                    s * (momentum.at(i) - momentum_copy.at(i))) / 2;
                    momentum.at(i) = (momentum.at(i) + momentum_copy.at(i) -
                                      s * (params.at(i) - params_copy.at(i)) +
                                      c * (momentum.at(i) - momentum_copy.at(i))) / 2;
                    params_copy.at(i) = (params.at(i) + params_copy.at(i) -
                                         c * (params.at(i) - params_copy.at(i)) -
                                         s * (momentum.at(i) - momentum_copy.at(i))) / 2;
                    momentum_copy.at(i) = (momentum.at(i) + momentum_copy.at(i) +
                                           s * (params.at(i) - params_copy.at(i)) -
                                           c * (momentum.at(i) - momentum_copy.at(i))) / 2;

                }

                foliation = ham(params_copy, momentum);
                dynamics = ham_grad(foliation);
                if (!dynamics.has_value()) {
                    error_msg();
                    break;
                }

                for (uint32_t i = 0; i < nparam; i++) {
                    params.at(i) = params.at(i) + std::get<1>(dynamics.value()).at(i) * delta;
                    momentum_copy.at(i) = momentum_copy.at(i) - std::get<0>(dynamics.value()).at(i) * delta;
                }

                foliation = ham(params, momentum_copy);
                dynamics = ham_grad(foliation);
                if (!dynamics.has_value()) {
                    error_msg();
                    break;
                }

                for (uint32_t i = 0; i < nparam; i++) {
                    params_copy.at(i) = params_copy.at(i) + std::get<1>(dynamics.value()).at(i) * delta;
                    momentum.at(i) = momentum.at(i) - std::get<0>(dynamics.value()).at(i) * delta;
                }

                foliation = ham(params, momentum);
                if (!foliation.has_value()) {
                    error_msg();
                    break;
                }

                params_flow.push_back(params);
                momentum_flow.push_back(momentum);
                energy_level.push_back(std::get<Energy>(foliation.value()).detach());

                if (iter_step < conf.max_flow_steps - 1) {
                    const auto rho = -torch::relu(energy_level.back() - energy_level.front());
                    if ((rho >= torch::log(torch::rand_like(rho))).item<bool>())
                        for (uint32_t i = 0; i < nparam; i++) {
                            params_copy.at(i) = params_copy.at(i) + std::get<1>(dynamics.value()).at(i) * delta;
                            momentum.at(i) = momentum.at(i) - std::get<0>(dynamics.value()).at(i) * delta;
                        }
                    else {
                        if (conf.verbose)
                            std::cout << "GHMC: rejecting sample at iteration "
                                      << iter_step + 1 << "/" << conf.max_flow_steps << "\n";
                        break;
                    }
                }
            }
            return HamiltonianFlow{params_flow, momentum_flow, energy_level};
        };
    }


    template<typename LogProbabilityDensity, typename Configurations>
    inline auto sampler(const LogProbabilityDensity &log_prob_density, const Configurations &conf) {
        const auto ham_flow = hamiltonian_flow(log_prob_density, conf);
        return [ham_flow, conf](const Parameters &initial_parameters, const uint32_t num_iterations) {
            const auto max_num_samples = conf.max_flow_steps * num_iterations;

            auto samples = Samples{};
            samples.reserve(max_num_samples + 1);

            if (conf.verbose)
                std::cout << "GHMC: Riemannian HMC simulation\n"
                          << "GHMC: generating MCMC chain of maximum length "
                          << max_num_samples << " ...\n";

            const auto nparam = initial_parameters.size();
            auto params = Parameters{};
            params.reserve(nparam);
            for (const auto &param : initial_parameters)
                params.push_back(param.detach());

            samples.push_back(params);
            uint32_t iter = 0;

            while (iter < num_iterations) {
                auto flow = ham_flow(samples.back());
                const auto &params_flow = std::get<0>(flow);
                if (params_flow.size() > 1)
                    samples.insert(samples.end(), params_flow.begin() + 1, params_flow.end());
                iter++;
            }

            if (conf.verbose)
                std::cout << "GHMC: generated "
                          << samples.size() << " samples.\n";

            return samples;
        };
    }

} // namespace noa::ghmc