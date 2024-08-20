module;

#include <cassert>

#include <enzyme/enzyme>

module local_vol;

import :configs;
import :euler_calc;
import :mat;

import std;

namespace local_vol {

void calc_pv_vegas(
    std::mdspan<double, ext2d> vegas,
    std::mdspan<const double, ext2d> sigmas,
    const MarketDataConfig& market_config,
    const ModelConfig& model_config,
    const Trade& trade
) {
    assert(vegas.size() == sigmas.size());
    assert(vegas.extents().extent(0) == sigmas.extents().extent(0));
    assert(vegas.extents().extent(1) == sigmas.extents().extent(1));

    const enzyme::Duplicated<const double*> data_arg{
        sigmas.data_handle(), vegas.data_handle()
    };

    enzyme::autodiff<enzyme::Reverse>(
        calc_pv_impl,
        data_arg,
        enzyme::Const{ sigmas.extents().extent(0) },
        enzyme::Const{ sigmas.extents().extent(1) },
        enzyme::Const<const MarketDataConfig&>{ market_config },
        enzyme::Const<const ModelConfig&>{ model_config },
        enzyme::Const<const Trade&>{ trade }
    );
} // <-- calc_pv_vegas

} // <-- namespace local_vol
