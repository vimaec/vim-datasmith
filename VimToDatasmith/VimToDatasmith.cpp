// Copyright (c) 2021 VIM
// Licensed under the MIT License 1.0

#include "VimToDatasmith.h"
#include "CVimToDatasmith.h"

#include "DebugTools.h"

DISABLE_SDK_WARNINGS_START

#include "DatasmithExporterManager.h"

DISABLE_SDK_WARNINGS_END

#if winOS
#define DirectorySeparator '\\'
#else
#define DirectorySeparator '/'
#endif

namespace Vim2Ds {

void Usage() {
    DebugF("Usage: VimToDatasmith VimFilePath.vim [DatasmithFilePath.udatasmith]");
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

static void InitDatasmith() {
    TraceF("InitDatasmith\n");
    FDatasmithExporterManager::FInitOptions Options;
    Options.bSuppressLogs = true; // DEBUG == 0;
    FDatasmithExporterManager::Initialize(Options);
}

int Convert(int argc, const utf8_t* argv[]) {
    int result = EXIT_FAILURE;
    try {
        InitDatasmith();
        CVimToDatasmith vimToDatasmith;
        vimToDatasmith.GetParameters(argc, argv);
        vimToDatasmith.ReadVimFile();
        vimToDatasmith.CreateDatasmithScene();
        vimToDatasmith.CreateDatasmithFile();
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
