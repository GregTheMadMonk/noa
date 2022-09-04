#pragma once

namespace noa::test {

template <typename Real, typename Func>
class CachedIntegrator {
        std::vector<Real> positive, negative;
        // std::function<Func> kernel;
        Func kernel;
        Real step;
        Real integrateZeroTo(const Real& to) {
                if (to == 0) return 0;

                auto& vec = positive;
                int sign = 1;
                if (to < 0) {
                        vec = negative;
                        sign = -1;
                }

                const size_t steps = to * sign / step;
                if (vec.size() >= steps) return vec[steps - 1];

                vec.reserve(steps);
                for (size_t i = vec.size(); i <= steps; ++i)
                        vec.push_back(vec.back() + kernel(sign * i * step) * sign * step);

                return vec.back();
        }
        public:
        CachedIntegrator(Func f, const Real& s) : kernel(f), step(s) {}

        void clear() {
                positive.clear();
                negative.clear();
        }
        Real integrate(const Real& from, const Real& to) {
                if (to == from) return 0;

                // Calculate integral between zero and `from` and zero and `to`
                return integrateZeroTo(to) - integrateZeroTo(from);
        }
}; // <-- class CachedIntegrator

} // <-- namespace noa::test
