// Copyright (c) 2021 VIM
// Licensed under the MIT License 1.0

#pragma once

#include "VimToDatasmith.h"

namespace Vim2Ds {

#ifndef __clang__
#define __printflike(a, b)
#endif

typedef enum { kP2DB_Report = 1, kP2DB_Debug, kP2DB_ReportAndDebug, kP2DB_Trace, kP2DB_Verbose } EP2DB;

void Printf2DB(EP2DB InMsgLevel, const utf8_t *FormatString, ...) __printflike(2, 3);

// Write string to log file
void Write2Log(EP2DB InMsgLevel, const utf8_string &InMsg);

[[noreturn]] void ThrowMessage(const utf8_t *MessageFormatString, ...);
[[noreturn]] void ThrowPtrNull(const utf8_t *InFile, int InLineNo);
[[noreturn]] void ThrowAssertionFail(const utf8_t *InFile, int InLineNo);

#if PLATFORM_WINDOWS

[[noreturn]] void ThrowWinError(DWORD winErr, const utf8_t *InFile, int InLineNo);

// This define throw an std::runtime_error
#define TestWinError()                                 \
    {                                                  \
        DWORD winErr = GetLastError();                 \
        if (winErr != 0) {                             \
            ThrowWinError(winErr, __FILE__, __LINE__); \
        }                                              \
    }

#endif

// This define test an pointer and on NULL, we throw an std::runtime_error
#define TestPtr(Ptr)                                  \
    {                                                 \
        if ((Ptr) == nullptr) {                       \
            Vim2Ds::ThrowPtrNULL(__FILE__, __LINE__); \
        }                                             \
    }

// This define test an assertion and on failure, we throw an std::runtime_error
#define TestAssert(Assertion)                               \
    {                                                       \
        if (!(Assertion)) {                                 \
            Vim2Ds::ThrowAssertionFail(__FILE__, __LINE__); \
        }                                                   \
    }

#if defined(DEBUG) || 0 // Usually only for debug, but can be forced for release by putting 1
#define TestAssertDebugOnly(...) TestAssert(__VA_ARGS__)
#else
#define TestAssertDebugOnly(...) (void)0
#endif

#define DebugF(...) Vim2Ds::Printf2DB(Vim2Ds::kP2DB_Debug, __VA_ARGS__)
#define TraceF(...) Vim2Ds::Printf2DB(Vim2Ds::kP2DB_Trace, __VA_ARGS__)
#if 0
#define VerboseF(...) Vim2Ds::Printf2DB(Vim2Ds::kP2DB_Verbose, __VA_ARGS__)
#else
#define VerboseF(...) (void)0
#endif

} // namespace Vim2Ds
