//
//  VimToDatasmith.h
//  VimToDatasmith
//
//  Created by Richard Young on 2021-05-17.
//

#pragma once

#include "VimToDsWarningsDisabler.h"

#define BeginVim2DatasmithNameSpace namespace Vim2Ds {
#define EndVim2DatasmithNameSpace }

#include <string>
#include <cmath>


class cVec3;
class cVec4;
class cMat4;
class ubyte4;
class cAABB;
class FQuat;
class FVector;

BeginVim2DatasmithNameSpace


// All code is utf8 based
typedef char		utf8_t;
typedef std::string utf8_string;

#ifndef __clang__
	#define __printflike(a, b)
#endif

// Format string
utf8_string VStringFormat(const utf8_t* InFmt, va_list InArgumentsList) __printflike(1, 0);;

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

// Return true if this is an identity matrix
bool IsIdentity(const cMat4& inTransform);

// Extract the rotation from the matrix and return as a Quat
FQuat GetRotationQuat(const cMat4& inTransform);

// Extract translation from the matrix and return an Unreal one (in centimeters)
FVector GetTranslationVector(const cMat4& inTransform);

// Extract scale from the matrix and return an Unreal one (in centimeters)
FVector GetScaleVector(const cMat4& inMatrix);


EndVim2DatasmithNameSpace
