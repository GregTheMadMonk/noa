#include "functions.hh"

#include <iostream>

#include <cblas.h>

#include <enzyme/enzyme>

void hi() { std::cout << "hi!\n"; }

double sum(const double* v, std::size_t size) noexcept {
    double ret = 0.0;
    for (std::size_t i = 0; i < size; ++i) ret += v[i];
    return ret;
} // <-- sum(v, size)

void   d_sum(const double* v, double* dv, std::size_t size) noexcept {
    enzyme::autodiff<enzyme::Reverse>(
        sum,
        enzyme::DuplicatedNoNeed<const double*>{ v, dv },
        enzyme::Const{ size }
    );
} // <-- d_sum(v, dv, size)

double dot(const double* a, const double* b, std::size_t size) noexcept {
    return cblas_ddot(size, a, 1, b, 1);
} // <-- dot(a, b, size)

// void* __enzyme_register_gradient

void   d_dot(
    const double* a, double* da,
    const double* b, double* db,
    std::size_t size
) noexcept {
    enzyme::autodiff<enzyme::Reverse>(
        dot,
        enzyme::DuplicatedNoNeed<const double*>{ a, da },
        enzyme::DuplicatedNoNeed<const double*>{ b, db },
        enzyme::Const{ size }
    );
} // <-- d_dot(a, da, b, db, size)
