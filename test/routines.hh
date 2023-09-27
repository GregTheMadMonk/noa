#pragma once

#include "test-data.hh"

#include <noa/ghmc.hh>
#include <noa/utils/common.hh>

#include <torch/torch.h>
#include <chrono>

using namespace std::chrono;
using namespace noa;
using namespace noa::ghmc;
using namespace noa::utils;

/*
 * Save result for analysis. One can explore the sample in python:
 * next(torch.jit.load(<@param save_result_to>).parameters())
 */
inline void save_result(const Tensor &sample, const Path &save_result_to) {
    std::cout << "Saving result to " << save_result_to << "\n";
    torch::save(sample, save_result_to);
}


inline Status sample_normal_distribution(const Path &save_result_to,
                                         torch::DeviceType device = torch::kCPU) {
    torch::manual_seed(SEED);

    // Sample from the 3 dimensional Gaussian distribution
    // HMC requires the log density up to additive constants
    const auto mean = torch::tensor({0.f, 10.f, 5.f}, torch::device(device));
    const auto sigma = torch::tensor({.5f, 1.f, 2.f}, torch::device(device));

    const auto log_prob_normal = [&mean, &sigma](const Parameters &theta_) {
        const auto theta = theta_.at(0).detach().requires_grad_(true);
        const auto log_prob = -((theta - mean) / sigma).pow(2).sum() / 2;
        return LogProbabilityGraph{log_prob, {theta}};
    };

    std::cout << "Sampling Normal distribution:\n"
              << " mean =\n  "
              << mean.view({1, 3}) << "\n sigma =\n  "
              << sigma.view({1, 3}) << "\n";

    // Initialise parameters
    const auto params_init = Parameters{torch::zeros(3, torch::device(device))};

    // Create sampler
    const auto conf = Configuration<float>{}
            .set_max_flow_steps(5)
            .set_step_size(0.3f).set_verbosity(true);
    const auto ham_dym = riemannian_dynamics(log_prob_normal, softabs_metric(conf), metropolis_criterion, conf);
    const auto normal_sampler = sampler(ham_dym, full_trajectory, conf);

    // Run sampler
    const auto begin = steady_clock::now();
    const auto samples = normal_sampler(params_init, 200);
    const auto end = steady_clock::now();
    std::cout << "GHMC: sampler took " << duration_cast<microseconds>(end - begin).count() / 1E+6
              << " seconds" << std::endl;
    const auto num_samples = samples.size();
    if (num_samples <= 1) {
        std::cerr << "Sampler failed\n";
        return false;
    }

    const auto result = stack(samples);
    save_result(result, save_result_to);
    const auto[s_sigma, s_mean] = torch::std_mean(result.slice(0, result.size(0) / 10, result.size(0)), 0, true, true);

    std::cout << "Sample statistics:\n"
              << " mean =\n  "
              << s_mean << "\n sigma =\n  "
              << s_sigma << "\n";

    return true;
}


inline Status sample_funnel_distribution(const Path &save_result_to,
                                         torch::DeviceType device = torch::kCPU) {
    torch::manual_seed(SEED);

    std::cout << "Sample from the 10 dimensional Funnel distribution:\n";

    // Initialise parameters
    auto params_init = torch::ones(11, torch::device(device));
    params_init[0] = 0.;

    // Create sampler
    const auto conf = Configuration<float>{}
            .set_max_flow_steps(25)
            .set_jitter(0.001f)
            .set_step_size(0.14f)
            .set_binding_const(10.f).set_verbosity(true);
    const auto ham_dym = riemannian_dynamics(log_funnel,  softabs_metric(conf), metropolis_criterion, conf);
    const auto funnel_sampler = sampler(ham_dym, full_trajectory, conf);

    // Run sampler
    const auto begin = steady_clock::now();
    const auto samples = funnel_sampler(Parameters{params_init}, 100);
    const auto end = steady_clock::now();
    std::cout << "GHMC: sampler took " << duration_cast<microseconds>(end - begin).count() / 1E+6
              << " seconds" << std::endl;
    if (samples.size() <= 1) {
        std::cerr << "Sampler failed\n";
        return false;
    }

    const auto result = stack(samples);
    save_result(result, save_result_to);

    return true;
}


inline Status sample_bayesian_net(const Path &save_result_to,
                                  torch::DeviceType device = torch::kCPU) {
    torch::manual_seed(SEED);

    std::cout << "Bayesian Deep Learning regression example:\n";

    auto module = load_module(jit_net_pt);
    if (!module.has_value())
        return false;

    const auto n_tr = 6;
    const auto n_val = 300;
    const auto n_epochs = 250;

    const auto x_val = torch::linspace(-5.f, 5.f, n_val, torch::device(device)).view({-1, 1});
    const auto y_val = torch::sin(x_val);

    const auto x_train = torch::linspace(-3.14f, 3.14f, n_tr, torch::device(device)).view({-1, 1});
    const auto y_train = torch::sin(x_train) + 0.1f * torch::randn_like(x_train);

    auto &net = module.value();
    net.train();
    net.to(device);

    const auto params_init = flat_parameters(net);
    const auto net_params = parameters(net);

    const auto zero_params = zeros_like(net_params, true);
    const auto log_prob_bnet = numerics::regression_log_probability(
            net, 0.01f, zero_params, 1.f)(x_train, y_train);


    const auto conf_bnet = Configuration<float>{}
            .set_max_flow_steps(25)
            .set_step_size(0.001f)
            .set_verbosity(true);

    const auto ham_dym = euclidean_dynamics(
            log_prob_bnet, identity_metric_like(net_params), metropolis_criterion, conf_bnet);
    const auto bnet_sampler = sampler(ham_dym, full_trajectory, conf_bnet);

    // Run sampler
    const auto begin = steady_clock::now();
    const auto samples = bnet_sampler(net_params, 300);
    const auto end = steady_clock::now();
    std::cout << "GHMC: sampler took " << duration_cast<microseconds>(end - begin).count() / 1E+6
              << " seconds" << std::endl;
    if (samples.size() <= 1) {
        std::cerr << "Sampler failed\n";
        return false;
    }

    const auto result = stack(samples);
    save_result(result, save_result_to);

    const auto stationary_sample = result.slice(0, result.size(0) / 2);

    set_flat_parameters(net, stationary_sample.mean(0));
    const auto posterior_mean_pred = net({x_val}).toTensor().detach();

    auto bayes_preds_ = Tensors{};
    bayes_preds_.reserve(stationary_sample.size(0));
    for (uint32_t i = 0; i < stationary_sample.size(0); i++) {
        set_flat_parameters(net, stationary_sample[i]);
        bayes_preds_.push_back(net({x_val}).toTensor().detach());
    }
    const auto bayes_preds = torch::stack(bayes_preds_);
    const auto bayes_mean_pred = bayes_preds.mean(0);
    const auto bayes_std_pred = bayes_preds.std(0);

    set_flat_parameters(net, params_init);
    auto loss_fn = torch::nn::MSELoss{};
    auto optimizer = torch::optim::Adam{net_params, torch::optim::AdamOptions(0.005)};
    const auto initial_loss = loss_fn(net({x_val}).toTensor().detach(), y_val);

    std::cout << " Running Adam gradient descent optimisation ...\n";
    for (uint32_t i = 0; i < n_epochs; i++) {

        optimizer.zero_grad();
        const auto output = net({x_train}).toTensor();
        const auto loss = loss_fn(output, y_train);
        loss.backward();
        optimizer.step();
    }

    std::cout << " Initial MSE loss:\n" << initial_loss << "\n"
              << " Optimal MSE loss:\n" << loss_fn(net({x_val}).toTensor().detach(), y_val) << "\n"
              << " Posterior mean MSE loss:\n" << loss_fn(posterior_mean_pred, y_val) << "\n"
              << " Bayes prediction mean MSE loss:\n" << loss_fn(bayes_mean_pred, y_val) << "\n"
              << " Bayes prediction +/- std MSE loss:\n"
              << torch::stack({loss_fn(bayes_mean_pred + bayes_std_pred, y_val),
                               loss_fn(bayes_mean_pred - bayes_std_pred, y_val)}).view({1,2})
              << "\n";

    return true;
}
