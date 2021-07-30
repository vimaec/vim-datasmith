// Copyright (c) 2021 VIM
// Licensed under the MIT License 1.0

#include "VimToDatasmith.h"
#include "CConvertVimToDatasmith.h"

#include "CTaskMgr.h"
#include "DebugTools.h"
#include "cAABB.h"
#include "cMat.h"

DISABLE_SDK_WARNINGS_START

#include "DatasmithExporterManager.h"

DISABLE_SDK_WARNINGS_END

#if winOS
#define DirectorySeparator '\\'
#else
#define DirectorySeparator '/'
#endif

#if winOS
extern "C" {
// Sometime it's hard to include "windows.h'" headers.
bool CreateDirectoryW(wchar_t* lpPathName, void* lpSecurityAttributes);
}
#endif

namespace Vim2Ds {

// Simple function to create a folder
bool CreateFolder(const utf8_t* inFolderName) {
    struct stat st = {0};
    if (stat(inFolderName, &st) == -1) {
#if winOS
        if (CreateDirectoryW(UTF8_TO_TCHAR(inFolderName), nullptr) != true) {
            DebugF("CreateFolder - Can't create folder: \"%s\" error=%d\n", inFolderName, errno);
            return false;
        }
#else
        if (mkdir(inFolderName, S_IRWXU | S_IRWXG | S_IRWXO) != 0) {
            DebugF("CreateFolder - Can't create folder: \"%s\" error=%d\n", inFolderName, errno);
            return false;
        }
#endif
    }
    return true;
}

void Usage() {
    DebugF("Usage: VimToDatasmith [-NoHierarchicalInstance] VimFilePath.vim [DatasmithFilePath.udatasmith]");
    exit(EXIT_FAILURE);
}

void ExtractPathNameExtension(const std::string& inFilePathName, std::string* outPath, std::string* outName, std::string* outExtension) {
    std::string::size_type posName = inFilePathName.find_last_of(DirectorySeparator) + 1;
    std::string::size_type posExtension = inFilePathName.find_last_of('.');
    if (posExtension >= posName) {
        if (outExtension != nullptr)
            *outExtension = inFilePathName.substr(posExtension);
    } else
        posExtension = std::string::npos;

    if (outName != nullptr)
        *outName = inFilePathName.substr(posName, posExtension - posName);

    if (outPath != nullptr && posName != 0)
        *outPath = inFilePathName.substr(0, posName - 1);
}

#if macOS && !defined(DEBUG)
#define RedirectOutput 1
#else
#define RedirectOutput 0
#endif

static void InitDatasmith() {
#if RedirectOutput
    // Redirect outputs to file InitDatasmith.txt
    int dupStdout = -1;
    int dupStderr = -1;
    int initDatasmithFile = open("InitDatasmith.txt", O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
    if (initDatasmithFile != -1) {
        dupStdout = dup(STDOUT_FILENO);
        dupStderr = dup(STDERR_FILENO);
        dup2(initDatasmithFile, STDOUT_FILENO);
        dup2(initDatasmithFile, STDERR_FILENO);
    }
#endif

    TraceF("InitDatasmith\n");
    FDatasmithExporterManager::FInitOptions Options;
    Options.bSuppressLogs = true; // DEBUG == 0;
    FDatasmithExporterManager::Initialize(Options);

#if RedirectOutput
    // Restore outputs to standard streams
    if (dupStdout != -1) {
        dup2(dupStdout, STDOUT_FILENO);
        close(dupStdout);
    }
    if (dupStderr != -1) {
        dup2(dupStderr, STDERR_FILENO);
        close(dupStderr);
    }
    if (initDatasmithFile != -1)
        close(initDatasmithFile);
#endif
}

int Convert(int argc, const utf8_t* argv[]) {
    int result = EXIT_FAILURE;
    try {
        InitDatasmith();
        CConvertVimToDatasmith vimToDatasmith;
        vimToDatasmith.GetParameters(argc, argv);
        vimToDatasmith.Convert();
        CTaskMgr::DeleteMgr();
        result = EXIT_SUCCESS;
    } catch (const std::exception& e) {
        DebugF("Convert - Exception %s\n", e.what());
    } catch (...) {
        DebugF("Convert - Unknown exception\n");
    }
    return result;
}

utf8_string ToString(int32_t inInt) {
    return Utf8StringFormat("%d", inInt);
}

utf8_string ToString(uint32_t inUInt) {
    return Utf8StringFormat("%u", inUInt);
}

utf8_string ToString(const cVec3& inVec3) {
    return Utf8StringFormat("%f, %f, %f", inVec3.x, inVec3.y, inVec3.z);
}

utf8_string ToString(const cVec4& inVec4) {
    return Utf8StringFormat("%f, %f, %f, %f", inVec4.x, inVec4.y, inVec4.z, inVec4.w);
}

utf8_string ToString(const cMat4& inTransform, const utf8_t* inSep) {
    return Utf8StringFormat("%s%s, %s%s, %s%s, %s%s", inSep, ToUtf8(inTransform.mRow0), inSep, ToUtf8(inTransform.mRow1), inSep, ToUtf8(inTransform.mRow2),
                            inSep, ToUtf8(inTransform.mRow3));
}

utf8_string ToString(const ubyte4& inColor) {
    return Utf8StringFormat("%d, %d, %d, %d", inColor.x, inColor.y, inColor.z, inColor.w);
}

utf8_string ToString(const cAABB& inAxisAlignedBoundingBox) {
    return Utf8StringFormat("{(%s), (%s)}", ToUtf8(inAxisAlignedBoundingBox.mMin), ToUtf8(inAxisAlignedBoundingBox.mMax));
}

// To convert from Vim meters to UE centimeter units
const float Meter2Centimeter = 100.0f;

} // namespace Vim2Ds
