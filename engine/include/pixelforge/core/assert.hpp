#pragma once
#include <pixelforge/core/logger.hpp>
#include <source_location>
#include <cstdlib>

namespace pf::detail {

inline void assert_fail(const char* expr, const char* msg,
                        const std::source_location& loc) {
    PF_LOG_FATAL("Assertion failed: {} — {} ({}:{}:{})",
        expr, msg ? msg : "", loc.file_name(), loc.line(), loc.function_name());
    std::abort();
}

} // namespace pf::detail

#define PF_ASSERT(expr, ...) \
    do { \
        if (!(expr)) { \
            ::pf::detail::assert_fail(#expr, \
                ([] { return "" __VA_OPT__(#__VA_ARGS__); }()), \
                std::source_location::current()); \
        } \
    } while(false)

#define PF_ASSERT_MSG(expr, msg) \
    do { \
        if (!(expr)) { \
            ::pf::detail::assert_fail(#expr, msg, std::source_location::current()); \
        } \
    } while(false)
