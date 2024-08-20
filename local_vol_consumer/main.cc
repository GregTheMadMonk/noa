import local_vol;
import rapidcsv;
import std;

int main(int argc, char** argv) {
    std::println("local_vol consumer exectuable");
    const auto wd = std::filesystem::path{ argv[0] }.parent_path();
    std::println("CSV dir: {}", wd.string());

    std::size_t N_PATHS = 1000;
    std::size_t N_DAYS  = 300;
    if (argc > 1) {
        N_PATHS = static_cast<std::size_t>(std::atoll(argv[1]));
        if (argc > 2) {
            N_DAYS = std::min(
                static_cast<std::size_t>(std::atoll(argv[2])),
                N_DAYS
            );
        }
    }

    rapidcsv::Document fwd_csv{
        wd / "fwd.csv", rapidcsv::LabelParams{ -1, -1 }
    };
    const std::vector<double> ttms = fwd_csv.GetColumn<double>(0);
    const std::size_t n_ttms = ttms.size();

    const double fwd = fwd_csv.GetColumn<double>(1).at(0);

    rapidcsv::Document impl_vol_csv{
        wd / "impl_vol.csv", rapidcsv::LabelParams{ -1, -1 }
    };

    std::vector<double> strikes = impl_vol_csv.GetColumn<double>(0);
    const std::size_t n_strikes = strikes.size();

    const auto sigmas = [&] {
        std::vector<double> ret;
        ret.reserve(n_ttms * n_strikes);
        for (auto col : std::views::iota(0uz, n_ttms)) {
            for (auto smile_i : impl_vol_csv.GetColumn<double>(col)) {
                ret.push_back(smile_i);
            }
        }
        return ret;
    } (); // <-- sigmas
    const std::mdspan sigmas_mds{ sigmas.data(), n_ttms, n_strikes };

    const local_vol::MarketDataConfig market_config{ ttms, strikes, fwd };

    const double Kc = 1.1 * fwd;
    const double Kp = 0.9 * fwd;
    const std::size_t TTMc = 2 * (N_DAYS / 3);
    const std::size_t TTMp = 4 * (N_DAYS / 5);

    const local_vol::Trade trade{
        local_vol::EuropeanCall{ Kc, 10, TTMc },
        local_vol::EuropeanPut { Kp, 10, TTMp },
    };

    const local_vol::ModelConfig model_config{ N_PATHS, N_DAYS };

    const auto pv = local_vol::calc_pv(
        sigmas_mds, market_config, model_config, trade
    );
    std::println("PV: {}", pv);

    std::vector<double> vegas(sigmas.size(), 0.0);
    std::mdspan vegas_mds{ vegas.data(), n_ttms, n_strikes };

    /*
    local_vol::calc_pv_vegas(
        vegas_mds, sigmas_mds, market_config, model_config, trade
    );
    */
    local_vol::hi();
    return 0;
}
