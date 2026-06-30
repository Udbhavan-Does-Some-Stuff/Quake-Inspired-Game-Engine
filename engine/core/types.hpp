// core/types.hpp
//
// Fixed-width type aliases and assertion macros used throughout the
// engine. Every other module includes this — it has zero dependencies
// of its own.

#pragma once

#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstdlib>

namespace core {

// ---- Fixed-width integer aliases ------------------------------------
using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using f32 = float;
using f64 = double;

using usize = size_t;

} // namespace core

// ---- Assertion macros -------------------------------------------------
// ASSERT is compiled out in release builds (NDEBUG defined).
// ASSERT_ALWAYS is never compiled out — use it for conditions that
// would corrupt engine state if violated (e.g. allocator bounds).

#if defined(NDEBUG)
    #define ASSERT(cond, msg) ((void)0)
#else
    #define ASSERT(cond, msg)                                              \
        do {                                                                \
            if (!(cond)) {                                                  \
                std::fprintf(stderr,                                        \
                    "[ASSERT] %s:%d  (%s)  %s\n",                           \
                    __FILE__, __LINE__, #cond, msg);                        \
                std::abort();                                               \
            }                                                               \
        } while (0)
#endif

#define ASSERT_ALWAYS(cond, msg)                                            \
    do {                                                                    \
        if (!(cond)) {                                                      \
            std::fprintf(stderr,                                           \
                "[FATAL] %s:%d  (%s)  %s\n",                                \
                __FILE__, __LINE__, #cond, msg);                            \
            std::abort();                                                   \
        }                                                                   \
    } while (0)
