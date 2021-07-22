// Copyright (c) 2021 VIM
// Licensed under the MIT License 1.0

#include "VimToDatasmith.h"

#if macOS

#import <Foundation/Foundation.h>

int main(int argc, const char* argv[]) {
    int result = EXIT_FAILURE;
    AutoReleasePool { result = Vim2Ds::Convert(argc, argv); }
    return result;
}

#elif winOS

DISABLE_SDK_WARNINGS_START
#include "Containers/StringConv.h"
DISABLE_SDK_WARNINGS_END

#include <vector>

int wmain(int argc, wchar_t* argv[], wchar_t* envp[]) {
    std::vector<Vim2Ds::utf8_string> parameters;
    for (int i = 0; i < argc; ++i)
        parameters.push_back(TCHAR_TO_UTF8(argv[i]));
    std::vector<const Vim2Ds::utf8_t*> paramArray;
    for (int i = 0; i < argc; ++i)
        paramArray.push_back(parameters[i].c_str());
    paramArray.push_back(nullptr);
    return Vim2Ds::Convert(argc, &paramArray[0]);
}

#endif
