// Copyright (c) 2021 VIM
// Licensed under the MIT License 1.0

#include "DebugTools.h"

#include "VimToDsWarningsDisabler.h"

#if PLATFORM_WINDOWS
DISABLE_SDK_WARNINGS_START
#include "Containers/StringConv.h"
DISABLE_SDK_WARNINGS_END
#endif

#include <cstdarg>
#include <stdexcept>

extern "C" {
typedef unsigned long DWORD;
enum { FORMAT_MESSAGE_FROM_SYSTEM = 0x00001000, FORMAT_MESSAGE_IGNORE_INSERTS = 0x00000200 };
#define MAKELANGID(a, b) 1 << 10
extern void OutputDebugStringW(const wchar_t*);
extern DWORD FormatMessageW(DWORD dwFlags, const char* lpSource, DWORD dwMessageId, DWORD dwLanguageId, wchar_t* lpBuffer, DWORD nSize, va_list* Arguments);
}

namespace Vim2Ds {

// Print in a string using the format and arguments list
utf8_string VStringFormat(const utf8_t* InFmt, va_list InArgumentsList) {
    // We need to copy in case of formatted string is bigger than default buffer
    va_list ArgumentsListCopy;
    va_copy(ArgumentsListCopy, InArgumentsList);
    // Try to print in a buffer
    utf8_t Buffer[10241];
    int FormattedStringSize = vsnprintf(Buffer, sizeof(Buffer), InFmt, InArgumentsList);
    if (FormattedStringSize < 0) {
        va_end(ArgumentsListCopy);
        throw std::runtime_error("vStringFormat - vsnprintf return an error");
    }

    // Is buffer was big enough ?
    if ((size_t)FormattedStringSize < sizeof(Buffer)) {
        va_end(ArgumentsListCopy);
        return Buffer;
    } else {
        // Buffer is too small for the resulting string, so we use the size
        // needed to create a right sized string
        utf8_string FormattedString;
        FormattedString.resize(FormattedStringSize);
        FormattedStringSize = vsnprintf((utf8_t*)FormattedString.c_str(), FormattedString.size() + 1, InFmt, ArgumentsListCopy);
        va_end(ArgumentsListCopy);
        if (FormattedStringSize < 0) {
            throw std::runtime_error("vStringFormat - vsnprintf return an error");
        }
        if ((size_t)FormattedStringSize != FormattedString.size()) {
            throw std::runtime_error("vStringFormat - vsnprintf return an inconsistant size");
        }

        return FormattedString;
    }
}

// Print in a string using the format and arguments
utf8_string Utf8StringFormat(const utf8_t* InFmt, ...) {
    utf8_string FormattedString;

    // Try to print in a buffer
    va_list ArgumentsList;
    va_start(ArgumentsList, InFmt);
    try {
        FormattedString = VStringFormat(InFmt, ArgumentsList);
    } catch (...) {
        // Clean up
        va_end(ArgumentsList);
        throw;
    }

    va_end(ArgumentsList);

    return FormattedString;
}

void ThrowMessage(const utf8_t* MessageFormatString, ...) {
    va_list argptr;
    va_start(argptr, MessageFormatString);
    utf8_string FormattedMessage(VStringFormat(MessageFormatString, argptr));
    va_end(argptr);
    throw std::runtime_error(FormattedMessage);
}

// Throw a runtime_error for null pointer
void ThrowPtrNULL(const utf8_t* InFile, int InLineNo) {
    utf8_t FormattedMessage[1024];
    snprintf(FormattedMessage, sizeof(FormattedMessage), "Pointer NULL at \"%s:%d\"", InFile, InLineNo);
    throw std::runtime_error(FormattedMessage);
}

// Throw a runtime_error for assertion fail
void ThrowAssertionFail(const utf8_t* InFile, int InLineNo) {
    utf8_t FormattedMessage[1024];
    snprintf(FormattedMessage, sizeof(FormattedMessage), "Assertion failed at \"%s:%d\"", InFile, InLineNo);
    throw std::runtime_error(FormattedMessage);
}

#if PLATFORM_WINDOWS

// Throw a runtime_error for last windows error
void ThrowWinError(unsigned long InWinErr, const utf8_t* InFile, int InLineNo) {
    wchar_t WinMsg[200];
    if (!FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, InWinErr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), WinMsg,
                        sizeof(WinMsg) / sizeof(WinMsg[0]), nullptr)) {
        WinMsg[0] = 0;
    }

    char FormattedMessage[1024];
    snprintf(FormattedMessage, sizeof(FormattedMessage), "Error %d=\"%s\" at \"%s:%d\"", InWinErr, TCHAR_TO_UTF8(WinMsg), InFile, InLineNo);
    throw std::runtime_error(FormattedMessage);
}

#endif

void Write2Log(EP2DB /*InMsgLevel*/, const utf8_string& /*InMsg*/) {
}

// Print to debugger
void Printf2DB(EP2DB InMsgLevel, const utf8_t* FormatString, ...) {
    try {
        va_list argptr;
        va_start(argptr, FormatString);
        utf8_string FormattedMessage(VStringFormat(FormatString, argptr));
        va_end(argptr);
        Write2Log(InMsgLevel, FormattedMessage);
#if PLATFORM_WINDOWS
        std::wstring WStr(UTF8_TO_TCHAR(FormattedMessage.c_str()));
        OutputDebugStringW(WStr.c_str());
        if (InMsgLevel == kP2DB_Debug) {
            fputws(WStr.c_str(), stderr);
        } else if (InMsgLevel <= kP2DB_Trace) {
            fputws(WStr.c_str(), stdout);
        }
#else
        if (fwrite(FormattedMessage.c_str(), 1, FormattedMessage.size(), stdout) != FormattedMessage.size()) {
            printf("Printf2DB - Write error %d\n", errno);
        }
#endif
    } catch (...) {
#if PLATFORM_WINDOWS
        OutputDebugStringW(L"Printf2DB - Catch an exception\n");
#else
        printf("Printf2DB - Catch an exception\n");
#endif
    }
}

} // namespace Vim2Ds
