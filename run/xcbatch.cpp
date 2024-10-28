// Copyright © 2022 Giorgio Audrito. All Rights Reserved.

/**
 * @file xcbatch.cpp
 * @brief Runs a single execution of the message dispatch case study with a graphical user interface.
 */
#include <iostream>

#include "lib/xc_processes.hpp"
#include "lib/xc_setup.hpp"

using namespace fcpp;

//! @brief Number of identical runs to be averaged.
constexpr int runs = 100;

int main() {
    // Construct the plotter object.
    option::plot_t p;
    // Parameter values
    int tvar = option::var_def<option::tvar>;
    int hops = option::var_def<option::hops>;
    int dens = option::var_def<option::dens>;
    int speed = option::var_def<option::speed>;
    int side = hops * (2*dens)/(2*dens+1.0) * comm / sqrt(2.0) + 0.5;
    int devices = dens*side*side/(3.141592653589793*comm*comm) + 0.5;
    double infospeed = (0.08*dens - 0.7) * speed * 0.01 + 0.075*dens*dens - 1.6*dens + 11;
    // The component type (batch simulator with given options).
    using comp_t = component::batch_simulator<option::list>;
    // The list of initialisation values to be used for simulations.
    auto init_list = batch::make_tagged_tuple_sequence(
            batch::arithmetic<option::seed>(1, runs, 1),
            batch::constant<option::tvar, option::dens, option::hops, option::speed, option::side, option::devices, option::infospeed, option::output, option::plotter>(
                tvar,
                dens,
                hops,
                speed,
                side,
                devices,
                infospeed,
                nullptr,
                &p
            )
    );
    // Runs the given simulations.
    batch::run(comp_t{}, init_list);
    // Builds the resulting plots.
    std::cout << plot::file("xcbatch", p.build(), {{"MAX_CROP", "0.5"}});
    return 0;
}
