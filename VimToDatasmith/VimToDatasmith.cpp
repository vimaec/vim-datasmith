//
//  VimToDatasmith.cpp
//  VimToDatasmith
//
//  Created by Richard Young on 2021-05-17.
//

#include "VimToDatasmith.h"
#include "CVimToDatasmith.h"

#include "DebugTools.h"

#include "cVec.h"
#include "cMat.h"

DISABLE_SDK_WARNINGS_START

#include "DatasmithExporterManager.h"

DISABLE_SDK_WARNINGS_END

#if Windows
	#define DirectorySeparator '\\'
#else
	#define DirectorySeparator '/'
#endif

BeginVim2DatasmithNameSpace


void Usage()
{
	DebugF("Usage: VimToDatasmith VimFilePath.vim [DatasmithFilePath.udatasmith]");
 	exit(EXIT_FAILURE);
}


void ExtractPathNameExtension(const std::string& inFilePathName, std::string* outPath, std::string* outName, std::string* outExtension)
{
	std::string::size_type posName = inFilePathName.find_last_of(DirectorySeparator) + 1;
	std::string::size_type posExtension = inFilePathName.find_last_of('.');
	if (posExtension >= posName) {
		if (outExtension != nullptr)
			*outExtension = inFilePathName.substr(posExtension);
	}
	else
		posExtension = std::string::npos;
	
	if (outName != nullptr)
		*outName = inFilePathName.substr(posName, posExtension - posName);
	
	if (outPath != nullptr && posName != 0)
		*outPath = inFilePathName.substr(0, posName - 1);
}


static void InitDatasmith()
{
	TraceF("InitDatasmith\n");
	FDatasmithExporterManager::FInitOptions Options;
	Options.bSuppressLogs = true; //DEBUG == 0;
	FDatasmithExporterManager::Initialize(Options);
}


int Convert(int argc, const utf8_t* argv[])
{
	int result = EXIT_FAILURE;
	try {
		InitDatasmith();
		CVimToDatasmith	vimToDatasmith;
		vimToDatasmith.GetParameters(argc, argv);
		vimToDatasmith.ReadVimFile();
		vimToDatasmith.CreateDatasmithScene();
		vimToDatasmith.CreateDatasmithFile();
		result = EXIT_SUCCESS;
	}
	catch (const std::exception& e) {
		DebugF("Convert - Exception %s\n", e.what());
	}
	catch (...) {
		DebugF("Convert - Unknown exception\n");
	}
	return result;
}


utf8_string ToString(int32_t inInt)
{
	return Utf8StringFormat("%d", inInt);
}

utf8_string ToString(uint32_t inUInt)
{
	return Utf8StringFormat("%u", inUInt);
}

utf8_string ToString(const cVec3& inVec3)
{
	return Utf8StringFormat("%f, %f, %f", inVec3.x, inVec3.y, inVec3.z);
}

utf8_string ToString(const cVec4& inVec4)
{
	return Utf8StringFormat("%f, %f, %f, %f", inVec4.x, inVec4.y, inVec4.z, inVec4.w);
}

utf8_string ToString(const cMat4& inTransform, const utf8_t* inSep)
{
	return Utf8StringFormat("%s%s, %s%s, %s%s, %s%s", inSep, ToUtf8(inTransform.mRow0), inSep, ToUtf8(inTransform.mRow1), inSep, ToUtf8(inTransform.mRow2), inSep, ToUtf8(inTransform.mRow3));
}

utf8_string ToString(const ubyte4& inColor)
{
	return Utf8StringFormat("%d, %d, %d, %d", inColor.x, inColor.y, inColor.z, inColor.w);
}

utf8_string ToString(const cAABB& inAxisAlignedBoundingBox)
{
	return Utf8StringFormat("{(%s), (%s)}", ToUtf8(inAxisAlignedBoundingBox.mMin), ToUtf8(inAxisAlignedBoundingBox.mMax));
}


// To convert from Vim meters to UE centimeter units
const float Meter2Centimeter = 100.0f;


// Return true if this is an identity matrix
bool IsIdentity(const cMat4& inTransform)
{
	return IsNearZero(inTransform.m16[0] - 1.0f) && IsNearZero(inTransform.m16[1]) && IsNearZero(inTransform.m16[2]) && IsNearZero(inTransform.m16[3]) &&
	IsNearZero(inTransform.m16[4]) && IsNearZero(inTransform.m16[5] - 1.0f) && IsNearZero(inTransform.m16[6]) && IsNearZero(inTransform.m16[7]) &&
	IsNearZero(inTransform.m16[8]) && IsNearZero(inTransform.m16[9]) && IsNearZero(inTransform.m16[10] - 1.0f) && IsNearZero(inTransform.m16[11]) &&
	IsNearZero(inTransform.m16[12]) && IsNearZero(inTransform.m16[13]) && IsNearZero(inTransform.m16[14]) && IsNearZero(inTransform.m16[15] - 1.0f);
}

// Extract the rotation from the matrix and return as a Quat
FQuat GetRotationQuat(const cMat4& inMatrix)
{
	double	RotAngle = 0.0;
	cVec3	RotAxis;
	if (IsNearZero(inMatrix.m01 - inMatrix.m10) &&
		IsNearZero(inMatrix.m02 - inMatrix.m20) &&
		IsNearZero(inMatrix.m12 - inMatrix.m21))
	{
		if (IsNearZero(inMatrix.m01 + inMatrix.m10, 0.1) &&
			IsNearZero(inMatrix.m02 + inMatrix.m20, 0.1) &&
			IsNearZero(inMatrix.m12 + inMatrix.m21, 0.1) &&
			IsNearZero(inMatrix.m00 + inMatrix.m11 + inMatrix.m22 - 3, 0.1))
		{
			// no rotation
			RotAngle = 0.0;
		}
		else
		{ // 180 degrees rotation
			RotAngle = PI;
			const double xx = (inMatrix.m00 + 1.0) * 0.5;
			const double yy = (inMatrix.m11 + 1.0) * 0.5;
			const double zz = (inMatrix.m22 + 1.0) * 0.5;
			const double xy = (inMatrix.m01 + inMatrix.m10) * 0.25;
			const double xz = (inMatrix.m02 + inMatrix.m20) * 0.25;
			const double yz = (inMatrix.m12 + inMatrix.m21) * 0.25;
			
			if ((xx > yy) && (xx > zz))
			{
				if (IsNearZero(xx))
				{
					RotAxis = cVec3(0.0f, 0.7071f, 0.7071f);
				}
				else
				{
					RotAxis[0] = sqrt(xx);
					RotAxis[1] = xy / RotAxis[0];
					RotAxis[2] = xz / RotAxis[0];
				}
			}
			else if (yy > zz)
			{
				if (IsNearZero(yy))
				{
					RotAxis = cVec3(0.7071f, 0.0f, 0.7071f);
				}
				else
				{
					RotAxis[1] = sqrt(yy);
					RotAxis[0] = xy / RotAxis[1];
					RotAxis[2] = yz / RotAxis[1];
				}
			}
			else
			{
				if (IsNearZero(zz))
				{
					RotAxis = cVec3(0.7071f, 0.7071f, 0.0f);
				}
				else
				{
					RotAxis[2] = sqrt(zz);
					RotAxis[0] = xz / RotAxis[2];
					RotAxis[1] = yz / RotAxis[2];
				}
			}
		}
	}
	else
	{
		RotAngle = acos((inMatrix.m00 + inMatrix.m11 + inMatrix.m22 - 1.0) * 0.5);
		RotAxis = cVec3((inMatrix.m21 - inMatrix.m12) / sqrt(sqr(inMatrix.m21 - inMatrix.m12) + sqr(inMatrix.m02 - inMatrix.m20) + sqr(inMatrix.m10 - inMatrix.m01)),
						(inMatrix.m02 - inMatrix.m20) / sqrt(sqr(inMatrix.m21 - inMatrix.m12) + sqr(inMatrix.m02 - inMatrix.m20) + sqr(inMatrix.m10 - inMatrix.m01)),
						(inMatrix.m10 - inMatrix.m01) / sqrt(sqr(inMatrix.m21 - inMatrix.m12) + sqr(inMatrix.m02 - inMatrix.m20) + sqr(inMatrix.m10 - inMatrix.m01)));
	}
	RotAxis.Normalise();
	
	return FQuat(FVector(RotAxis.x, -RotAxis.y, RotAxis.z), RotAngle).Inverse();
}


// Extract translation from the matrix and return an Unreal one (in centimeters)
FVector GetTranslationVector(const cMat4& inMatrix)
{
	return FVector(inMatrix.m03 * Meter2Centimeter, -inMatrix.m13 * Meter2Centimeter, inMatrix.m23 * Meter2Centimeter);
}


// Extract scale from the matrix and return an Unreal one (in centimeters)
FVector GetScaleVector(const cMat4& inMatrix)
{
	return FVector(sqrt(sqr(inMatrix.m00) + sqr(inMatrix.m10) + sqr(inMatrix.m20)),
				   sqrt(sqr(inMatrix.m01) + sqr(inMatrix.m11) + sqr(inMatrix.m21)),
				   sqrt(sqr(inMatrix.m02) + sqr(inMatrix.m12) + sqr(inMatrix.m22)));
}


EndVim2DatasmithNameSpace
