/**
 * @file unreachable.hh
 * @brief Defines `unreachable()` function
 */
#pragma once

// Standard library
#include <source_location>
#include <stdexcept>
#include <string>
#include <string_view>

namespace noa::utils {

/**
 * @brief Sign that the control has reached a part of the code that is not
 *        supposed to be reached normally.
 *
 * Unlike C++23's `std::unreachable()` which invokes undefined behavior
 * and provieds compiler optimizations, this function's only purpose it
 * to throw an @ref UnreachableCode exception.
 *
 * TODO: Maybe, throw in debug and use `__builtin_unreachable` in release
 * mode?
 *
 * @param error additional error message
 */
[[noreturn]] inline void unreachable(
    std::string_view error = {},
    std::source_location = std::source_location::current()
);

/**
 * @brief Exception thrown by @ref unreachable() signaling that control
 *        reached a part of code that was not supposed to be reached when
 *        operating normally
 */
class UnreachableCode : std::runtime_error {
    UnreachableCode(std::string_view err, std::source_location loc)
    : std::runtime_error {
        std::string{"Unreachable code encountered at ["}
        + loc.file_name() + ":" + std::to_string(loc.line()) + ":"
        + std::to_string(loc.column()) + "]" + (
            err.empty()
            ? std::string{}
            : (std::string{": "} + err.data())
        )
    } {}

    [[noreturn]] friend inline void
    unreachable(std::string_view err, std::source_location loc) {
        throw UnreachableCode{err, loc};
    } // <-- void unreachable(err, loc)
}; // <-- struct UnreachableCode

} // <-- namespace noa::utils
