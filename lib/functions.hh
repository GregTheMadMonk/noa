#include <cstddef>

extern void hi();

extern double sum(const double* v, std::size_t size) noexcept;
extern void   d_sum(const double* v, double* dv, std::size_t size) noexcept;

// Uses BLAS
extern double dot(const double* a, const double* b, std::size_t size) noexcept;
extern void   d_dot(
    const double* a, double* da,
    const double* b, double* db,
    std::size_t size
) noexcept;
