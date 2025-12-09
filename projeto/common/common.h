#pragma once

#include <stdarg.h>
#include "CommonTypes.h"
#include "CommonFuncs.h"

#ifndef DISALLOW_COPY_AND_ASSIGN
#define DISALLOW_COPY_AND_ASSIGN(t) \
    t(const t &other) = delete; \
    void operator =(const t &other) = delete;
#endif

#ifndef ENUM_CLASS_BITOPS
#define ENUM_CLASS_BITOPS(T) \
    static inline T operator |(const T &lhs, const T &rhs) { return T((int)lhs | (int)rhs); } \
    static inline T &operator |= (T &lhs, const T &rhs) { lhs = lhs | rhs; return lhs; } \
    static inline bool operator &(const T &lhs, const T &rhs) { return ((int)lhs & (int)rhs) != 0; } \
    static inline T operator ~(const T &rhs) { return (T)(~((int)rhs)); }
#endif

#define CHECK_HEAP_INTEGRITY()

#ifndef _WIN32
#ifndef MAX_PATH
#define MAX_PATH 260
#endif
#define __forceinline inline __attribute__((always_inline))
#endif
