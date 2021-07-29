// Copyright (c) 2021 VIM
// Licensed under the MIT License 1.0

#pragma once

#include "VimToDsWarningsDisabler.h"

#include <cmath>
#include <string>

class cVec3;
class cVec4;
class cMat4;
class ubyte4;
class cAABB;

namespace Vim2Ds {

// All code is utf8 based
typedef char utf8_t;
typedef std::string utf8_string;

#ifndef __clang__
#define __printflike(a, b)
#endif

#if macOS
#define AutoReleasePool @autoreleasepool
#else
#define AutoReleasePool
#endif

// Format string
utf8_string VStringFormat(const utf8_t* InFmt, va_list InArgumentsList) __printflike(1, 0);
;

// Print in a string using the format and arguments
utf8_string Utf8StringFormat(const utf8_t* InFmt, ...) __printflike(1, 2);

// Print program usage an exit
[[noreturn]] void Usage();

// Decompose the file path name in it's parts
void ExtractPathNameExtension(const utf8_string& inFilePathName, utf8_string* outPath, utf8_string* outName, utf8_string* outExtension);

// Convert function
int Convert(int argc, const utf8_t* argv[]);

// Simple tool to convert data to it's text representation
#define ToUtf8(v) ToString(v).c_str()

utf8_string ToString(int32_t inInt);

utf8_string ToString(uint32_t inUInt);

utf8_string ToString(const cVec3& inVec3);

utf8_string ToString(const cVec4& inVec4);

utf8_string ToString(const cMat4& inTransform, const utf8_t* inSep = "\n\t");

utf8_string ToString(const ubyte4& inColor);

utf8_string ToString(const cAABB& inAxisAlignedBoundingBox);

// Return true if the value is near zero (relative to epsilon provided
inline bool IsNearZero(float inValue, float inEps = 0.001f) {
    return fabsf(inValue) < inEps;
}

// Return the square of the value
inline float sqr(float inValue) {
    return inValue * inValue;
}

// To convert from Vim meters to UE centimeter units
extern const float Meter2Centimeter;

// Here are all VIM index. Use of emum to get some type checking
enum ElementIndex : uint32_t { kNoElement = (uint32_t)-1 };
enum NodeIndex : uint32_t { kNoNode = (uint32_t)-1 };
enum GeometryIndex : uint32_t { kNoGeometry = (uint32_t)-1 };
enum ParentIndex : uint32_t { kNoParent = (uint32_t)-1 };
enum VertexIndex : uint32_t { kInvalidVertex = (uint32_t)-1 };
enum GroupIndex : uint32_t { kInvalidGroup = (uint32_t)-1 };
enum MaterialId : uint32_t { kInvalidMaterial = (uint32_t)-1 };
enum IndiceIndex : uint32_t { kInvalidIndice = (uint32_t)-1 };
enum FaceIndex : uint32_t { kInvalidFace = (uint32_t)-1 }; // FaceIndex is same as Indices index / 3
enum StringIndex : uint32_t { kInvalidString = (uint32_t)-1 };

// Simple template function to increment enum value
template <class Enum> inline void Increment(Enum& e) {
    e = Enum(e + 1);
}

} // namespace Vim2Ds
