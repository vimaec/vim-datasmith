// Copyright (c) 2021 VIM
// Licensed under the MIT License 1.0

#include "CVimToDatasmith.h"

#include "DatasmithHashTools.h"
#include "DebugTools.h"

DISABLE_SDK_WARNINGS_START

#include "DatasmithExporterManager.h"
#include "DatasmithMesh.h"
#include "DatasmithMeshExporter.h"
#include "DatasmithSceneExporter.h"
#include "DatasmithSceneFactory.h"
#include "DatasmithSceneValidator.h"
#include "Math/Transform.h"
#include "Paths.h"

DISABLE_SDK_WARNINGS_END

#include <iostream>

namespace Vim2Ds {

// Constructor
CVimToDatasmith::MaterialEntry::MaterialEntry(int32_t inVimId)
: mCount(0)
, mVimId(inVimId)
, mMaterialElement(FDatasmithSceneFactory::CreateUEPbrMaterial(UTF8_TO_TCHAR(Utf8StringFormat("%d", inVimId).c_str()))) {
}

// Copy constructor required to be in a std::vector
CVimToDatasmith::MaterialEntry::MaterialEntry(const MaterialEntry& inOther)
: mCount((int32_t)inOther.mCount)
, mVimId(inOther.mVimId)
, mMaterialElement(inOther.mMaterialElement)
, mColor(inOther.mColor)
, mParams(inOther.mParams) {
}

// Constructor
CVimToDatasmith::CVimToDatasmith() {
    mStartTimeStat.ReStart();
    mGeometryToDatasmithMeshMap[kNoGeometry] = TSharedPtr<IDatasmithMeshElement>();
}

// Destructor
CVimToDatasmith::~CVimToDatasmith() {
}

// Parse parameters to get Vim file path and datasmith file path
void CVimToDatasmith::GetParameters(int argc, const utf8_t* const* argv) {
    if (argc > 1 && argv[1][0] == '-') {
        if (strcmp(argv[1], "-NoHierarchicalInstance") == 0) {
            mNoHierarchicalInstance = true;
            --argc;
            ++argv;
        }
    }

    if (argc < 2 || argc > 3)
        Usage();

    // Process input vim file argument
    mVimFilePath = argv[1];

    std::string vimPath;
    std::string vimName;
    std::string vimExtension;
    ExtractPathNameExtension(mVimFilePath, &vimPath, &vimName, &vimExtension);

    if (vimExtension != ".vim")
        ThrowMessage("Invalid vim file name \"%s\"", mVimFilePath.c_str());

    // Process output datassmith file argument
    if (argc > 2) {
        std::string datasmithExtension;
        ExtractPathNameExtension(argv[2], &mDatasmithFolderPath, &mDatasmithFileName, &datasmithExtension);
        if (datasmithExtension.size() != 0 && datasmithExtension != ".udatasmith")
            ThrowMessage("Invalid datasmith file name \"%s\"", argv[2]);
        if (mDatasmithFileName.size() == 0)
            mDatasmithFileName = vimName;
    } else {
        mDatasmithFolderPath = vimPath;
        mDatasmithFileName = vimName;
    }

    mOutputPath = UTF8_TO_TCHAR((mDatasmithFolderPath + "/" + mDatasmithFileName + "_Assets").c_str());

    DebugF("Convert \"%s\" -> \"%s\"\n", mVimFilePath.c_str(), (mDatasmithFolderPath + "/" + mDatasmithFileName + ".udatasmith").c_str());
}

// Initialize the Vim scene with the vim file
void CVimToDatasmith::ReadVimFile() {
    mSetupTimeStat.ReStart();
    Vim::VimErrorCodes vimReadResult = mVimScene.ReadFile(mVimFilePath);
    if (vimReadResult != Vim::VimErrorCodes::Success)
        ThrowMessage("CVimToDatasmith::ReadVimFile - ReadFile return error %d", vimReadResult);
    mReadTimeStat.ReStart();
}

void CVimToDatasmith::DumpTable(const utf8_t* inMsg, const Vim::EntityTable& inTable, bool inContent) const {
    for (auto iter : inTable.mIndexColumns) {
        TraceF("[\"%s\"] - mIndexColumns[%s] size=%lu\n", inMsg, iter.first.c_str(), iter.second.size());
        if (inContent)
            for (size_t i = 0; i < iter.second.size(); ++i)
                TraceF("\t[\"%s\"] [\"%s\"] [%lu] %d\n", inMsg, iter.first.c_str(), i, iter.second[i]);
    }
    for (auto iter : inTable.mStringColumns) {
        TraceF("[\"%s\"] - mStringColumns[%s] size=%lu\n", inMsg, iter.first.c_str(), iter.second.size());
        if (inContent)
            for (size_t i = 0; i < iter.second.size(); ++i)
                TraceF("\t[\"%s\"] [\"%s\"] [%lu] \"%s\"\n", inMsg, iter.first.c_str(), i, GetVimString(iter.second[i]));
    }
    for (auto iter : inTable.mNumericColumns) {
        TraceF("[\"%s\"] - mNumericColumns[%s] size=%lu\n", inMsg, iter.first.c_str(), iter.second.size());
        if (inContent)
            for (size_t i = 0; i < iter.second.size(); ++i)
                TraceF("\t[\"%s\"] [\"%s\"] [%lu] %lf\n", inMsg, iter.first.c_str(), i, iter.second[i]);
    }
    TraceF("\n");
}

// Create datasmith materials from Vim ones
void CVimToDatasmith::CreateMaterials() {
    TraceF("CVimToDatasmith::CreateMaterials\n");
    Vim::EntityTable& materialTable = mVimScene.mEntityTables["table:Rvt.Material"];
#if 0
    //    DumpTable(materialTable);
    for (auto table : mVimScene.mEntityTables)
        DumpTable(table.first.c_str(), table.second, true);

        /*
        const std::vector<double>& colorUvScalingX = materialTable.mNumericColumns["ColorUvScaling.X"];
        const std::vector<double>& colorUvScalingY = materialTable.mNumericColumns["ColorUvScaling.Y"];
        const std::vector<double>& colorUvOffsetX = materialTable.mNumericColumns["ColorUvOffset.X"];
        const std::vector<double>& colorUvOffsetY = materialTable.mNumericColumns["ColorUvOffset.Y"];
        const std::vector<double>& normalUvScalingX = materialTable.mNumericColumns["NormalUvScaling.X"];
        const std::vector<double>& normalUvScalingY = materialTable.mNumericColumns["NormalUvScaling.Y"];
        const std::vector<double>& normalUvOffsetX = materialTable.mNumericColumns["NormalUvOffset.X"];
        const std::vector<double>& normalUvOffsetY = materialTable.mNumericColumns["NormalUvOffset.Y"];
        const std::vector<double>& normalAmount = materialTable.mNumericColumns["NormalAmount"];
        for (size_t i = 0; i < colorUvScalingX.size(); ++i)
            TraceF("CVimToDatasmith::CreateMaterials - Color UvScaling(%lf, %lf) UvOffset(%lf, %lf) Normal UvScaling(%lf, %lf) UvOffset(%lf, %lf)
        Amount(%lf)\n", colorUvScalingX[i], colorUvScalingY[i], colorUvOffsetX[i], colorUvOffsetY[i], normalUvScalingX[i], normalUvScalingY[i],
        normalUvOffsetX[i], normalUvOffsetY[i], normalAmount[i]);
         */
#endif
    const std::vector<int>& nameArray = materialTable.mStringColumns["Name"];
    const std::vector<int>& textureNameArray = materialTable.mStringColumns["ColorTextureFile"];

    const std::vector<double>& idArray = materialTable.mNumericColumns["Id"];
    const std::vector<double>& colorXArray = materialTable.mNumericColumns["Color.X"];
    const std::vector<double>& colorYArray = materialTable.mNumericColumns["Color.Y"];
    const std::vector<double>& colorZArray = materialTable.mNumericColumns["Color.Z"];
    const std::vector<double>& transparencyArray = materialTable.mNumericColumns["Transparency"];
    const std::vector<double>& glossinessArray = materialTable.mNumericColumns["Glossiness"];
    const std::vector<double>& smoothnessArray = materialTable.mNumericColumns["Smoothness"];

    for (size_t i = 0; i < idArray.size(); i++) {
        uint32_t vimMaterialId = (uint32_t)idArray[i];

        VerboseF("CVimToDatasmith::CreateMaterials - MaterialId = %u\n", vimMaterialId);

        const utf8_t* textureName = GetVimString(textureNameArray, i);
        ;

        TestAssert(mVimToDatasmithMaterialMap.find(vimMaterialId) == mVimToDatasmithMaterialMap.end());
        mVimToDatasmithMaterialMap[vimMaterialId] = mMaterials.size();

        CVimToDatasmith::MaterialEntry materialEntry(vimMaterialId);

        materialEntry.mColor.x = colorXArray.size() > i ? (float)colorXArray[i] : 1.0f;
        materialEntry.mColor.y = colorYArray.size() > i ? (float)colorYArray[i] : 1.0f;
        materialEntry.mColor.z = colorZArray.size() > i ? (float)colorZArray[i] : 1.0f;
        materialEntry.mColor.w = transparencyArray.size() > i ? 1.0f - (float)transparencyArray[i] : 1.0f;

        materialEntry.mParams.x = glossinessArray.size() > i ? (float)glossinessArray[i] / 256.0f : 0.5f;
        materialEntry.mParams.y = smoothnessArray.size() > i ? (float)smoothnessArray[i] / 256.0f : 50.0f / 256.0f;

        IDatasmithUEPbrMaterialElement& element = materialEntry.mMaterialElement.Get();
        const utf8_t* materialName = GetVimString(nameArray, i);
        element.SetLabel(UTF8_TO_TCHAR((*materialName ? materialName : Utf8StringFormat("Vim %d", vimMaterialId).c_str())));
        IDatasmithMaterialExpressionColor* DiffuseExpression = element.AddMaterialExpression<IDatasmithMaterialExpressionColor>();
        if (DiffuseExpression != nullptr) {
            FLinearColor dsColor(materialEntry.mColor.x, materialEntry.mColor.y, materialEntry.mColor.z, materialEntry.mColor.w);
            DiffuseExpression->GetColor() = dsColor;
            DiffuseExpression->SetName(TEXT("Base Color"));
            DiffuseExpression->ConnectExpression(element.GetBaseColor());
            if (materialEntry.mColor.w < 0.999)
                DiffuseExpression->ConnectExpression(element.GetOpacity(), 3);
        }
        // material.mColor.w = transparencyArray.size() > i ? 1.0f -
        // (float)transparencyArray[i] : 1.0f; material.mParams.x =
        // glossinessArray.size() > i ? (float)glossinessArray[i] / 256.0f :
        // 0.5f; material.mParams.y = smoothnessArray.size() > i ?
        // (float)smoothnessArray[i] / 256.0f : 50.0f / 256.0f;

        mMaterials.push_back(materialEntry);
    }
}

// Convert geometry to Datasmith Mesh
void CVimToDatasmith::ConvertGeometryToDatasmithMesh(int32 geometryIndex, FDatasmithMesh* outMesh,
                                                     MapVimMaterialIdToDsMeshMaterialIndice* outVimMaterialIdToDsMeshMaterialIndice) {
    outMesh->SetName(UTF8_TO_TCHAR(Utf8StringFormat("%d", geometryIndex).c_str()));

    // Collect vertex used by this geometry
    std::unordered_map<uint32_t, int32_t> vimIndiceToDsMeshIndice;
    int32_t verticesCount = 0;
    uint32_t indicesStart = mGroupIndexOffets[geometryIndex];
    uint32_t indicesEnd = indicesStart + mGroupIndexCounts[geometryIndex];
    for (uint32_t index = indicesStart; index < indicesEnd; ++index) {
        uint32_t vertexIndex = mIndices[index];
        if (vimIndiceToDsMeshIndice.find(vertexIndex) == vimIndiceToDsMeshIndice.end())
            vimIndiceToDsMeshIndice[vertexIndex] = verticesCount++;
    }

    // Copy used vertex to the mesh
    outMesh->SetVerticesCount(verticesCount);
    for (const auto& iter : vimIndiceToDsMeshIndice) {
        const cVec3& position = mPositions[iter.first];
        outMesh->SetVertex(iter.second, position.x * Meter2Centimeter, -position.y * Meter2Centimeter, position.z * Meter2Centimeter);
    }

    // Collect materials used by this geometry
    int32_t materialsCount = 0;
    uint32_t vimMaterial = indicesStart / 3;

    // Copy faces used by this geometry
    int32_t facesCount = mGroupIndexCounts[geometryIndex] / 3;
    TestAssert(facesCount * 3 == mGroupIndexCounts[geometryIndex]);
    outMesh->SetFacesCount(facesCount);
    uint32_t vimIndice = indicesStart;
    for (int32_t indexFace = 0; indexFace < facesCount; ++indexFace) {
        // Get material
        auto insertResult = outVimMaterialIdToDsMeshMaterialIndice->insert({mMaterialIds[vimMaterial++], materialsCount});
        if (insertResult.second)
            ++materialsCount;

        // Get the face local vertices index.
        int32_t triangleVertices[3];
        cVec3 normals[3];
        for (int i = 0; i < 3; ++i) {
            uint32_t indice = mIndices[vimIndice++];
            triangleVertices[i] = vimIndiceToDsMeshIndice[indice];
            const cVec3& normal = mNormals[indice];
            outMesh->SetNormal(indexFace * 3 + i, normal.x, -normal.y, normal.z);
        }

        outMesh->SetFace(indexFace, triangleVertices[0], triangleVertices[1], triangleVertices[2], insertResult.first->second);

        /*

         int32_t triangleUVs[3];
         ...
                 outMesh->SetFaceUV(indexFace, UVChannel, triangleUVs[0], triangleUVs[1],
         triangleUVs[2]);
         */
    }
}

// In old vim files, the geometry is exported in world space, even when
// instanced, so we need to remove that world transform from the geometry
void CVimToDatasmith::FixOldVimFileTransforms() {
    if (mVimScene.mVersionMajor == 0 && mVimScene.mVersionMinor == 0 && mVimScene.mVersionPatch <= 200) {
        std::vector<bool> isTransformed(mGroupVertexOffets.Count(), false);

        for (size_t nodeIndex = 0; nodeIndex < mVimScene.mNodes.size(); ++nodeIndex) {
            const Vim::SceneNode& node = mVimScene.mNodes[nodeIndex];
            if (node.mGeometry != kNoGeometry) {
                TestAssert(node.mGeometry < (int)isTransformed.size());
                if (!isTransformed[node.mGeometry]) {
                    isTransformed[node.mGeometry] = true;
                    TestAssert(node.mInstance == nodeIndex || node.mInstance == kNoInstance);

                    cMat4 trans = (*(cMat4*)node.mTransform);
                    cMat4 invTrans = trans.Inverse();

                    uint32_t nextOffset =
                        node.mGeometry < (int)mGroupVertexOffets.Count() ? mGroupVertexOffets[node.mGeometry + 1] : uint32_t(mPositions.Count());
                    for (uint32_t index = mGroupVertexOffets[node.mGeometry]; index < nextOffset; ++index) {
                        cVec3& vertex = mPositions[index];
                        vertex = vertex * invTrans;
                    }
                }
            }
        }
    }
}

// Initialize the converter from Vim scene
void CVimToDatasmith::ProcessGeometry() {
    TraceF("CVimToDatasmith::ProcessGeometry\n");

    TAttributeVector<cVec4> colorAttribute;

    for (const g3d::Attribute& attr : mVimScene.mGeometry.attributes) {
        auto descriptorString = attr.descriptor.to_string();
        if (descriptorString == g3d::descriptors::VertexColorWithAlpha)
            colorAttribute.Initialize(attr);
        else if (descriptorString == g3d::descriptors::Position)
            mPositions.Initialize(attr);
        else if (descriptorString == g3d::descriptors::Index)
            mIndices.Initialize(attr);
        else if (descriptorString == g3d::descriptors::FaceMaterialId)
            mMaterialIds.Initialize(attr);
        else if (descriptorString == g3d::descriptors::FaceGroupId)
            mObjectIds.Initialize(attr);
        else if (descriptorString == g3d::descriptors::GroupIndexOffset)
            mGroupIndexOffets.Initialize(attr);
        else if (descriptorString == g3d::descriptors::GroupVertexOffset)
            mGroupVertexOffets.Initialize(attr);
        else if (descriptorString == g3d::descriptors::VertexUv)
            mVertexUVs.Initialize(attr);
        else
            TraceF("Unprocessed attribute \"%s\"\n", descriptorString.c_str());
    }

    const size_t groupCount = mGroupIndexOffets.Count();
    TestAssert(groupCount == mGroupVertexOffets.Count());

    // Calculate group index counts
    mGroupIndexCounts.Allocate(groupCount);
    if (groupCount > 0) {
        for (uint32_t i = 0; i < groupCount - 1; ++i)
            mGroupIndexCounts[i] = mGroupIndexOffets[i + 1] - mGroupIndexOffets[i];
        mGroupIndexCounts[groupCount - 1] = uint32_t(mIndices.Count()) - mGroupIndexOffets[groupCount - 1];
    }

    FixOldVimFileTransforms();

    ComputeNormals(true);
}

// Datasmith need normals.
void CVimToDatasmith::ComputeNormals(bool inFlip) {
    mNormals.Allocate(mPositions.Count());
    for (uint32_t i = 0; i < mIndices.Count(); i += 3) {
        cVec3 v0 = mPositions[mIndices[i + 0]];
        cVec3 v1 = mPositions[mIndices[i + 1]];
        cVec3 v2 = mPositions[mIndices[i + 2]];

        cVec3 s0 = v2 - v0;
        cVec3 s1 = v2 - v1;

        cVec3 normal = s1 ^ s0;
        normal.Normalise();
        if (inFlip)
            normal = -normal;

        mNormals[mIndices[i + 0]] += normal;
        mNormals[mIndices[i + 1]] += normal;
        mNormals[mIndices[i + 2]] += normal;
    }
    for (uint32_t i = 0; i < mIndices.Count(); ++i)
        mNormals[mIndices[i]].Normalise();
}

// Create all definitions
void CVimToDatasmith::ProcessDefinitions() {
    TraceF("CVimToDatasmith::ProcessDefinitions\n");
    mGeometryEntries.resize(mGroupIndexOffets.Count());

    for (size_t nodeIndex = 0; nodeIndex < mVimScene.mNodes.size(); nodeIndex++) {
        const Vim::SceneNode& node = mVimScene.mNodes[nodeIndex];

        // Get instance geometry definition
        if (node.mGeometry != kNoGeometry && (node.mInstance == kNoInstance || node.mInstance == nodeIndex)) {
            TestAssert((size_t)node.mGeometry < mGeometryEntries.size());
            std::unique_ptr<CGeometryEntry>& geometryEntry = mGeometryEntries[node.mGeometry];
            if (geometryEntry == nullptr)
                geometryEntry.reset(new CGeometryEntry(this, node));
            else
                geometryEntry->AddInstance(&node);
        }
    }
}

// Create all instances
void CVimToDatasmith::ProcessInstances() {
    TraceF("CVimToDatasmith::ProcessInstances\n");

    for (size_t nodeIndex = 0; nodeIndex < mVimScene.mNodes.size(); nodeIndex++) {
        const Vim::SceneNode& node = mVimScene.mNodes[nodeIndex];

        // Get instance geometry definition index
        if (node.mInstance != kNoInstance && node.mInstance != nodeIndex) {
            TestAssert((size_t)node.mInstance < mVimScene.mNodes.size());
            int geometryIndex = mVimScene.mNodes[node.mInstance].mGeometry;
#ifdef DEBUG
            if (node.mGeometry != kNoGeometry) {
                TraceF("CVimToDatasmith::ProcessInstances - Instance with geometry\n");
            }
#endif

            if (geometryIndex != kNoGeometry) {
                TestAssert((size_t)geometryIndex < mGeometryEntries.size());
                std::unique_ptr<CGeometryEntry>& geometryEntry = mGeometryEntries[geometryIndex];
                if (geometryEntry == nullptr)
                    geometryEntry.reset(new CGeometryEntry(this, node));
                else
                    geometryEntry->AddInstance(&node);
            }
        }
    }
}

// Create all actors
void CVimToDatasmith::CreateActors() {
    TraceF("CVimToDatasmith::CreateActors\n");

    for (auto& geometry : mGeometryEntries)
        if (geometry != nullptr)
            geometry->CreateActors();
}

// Add Datasmith materials used to the scene
void CVimToDatasmith::AddUsedMaterials() {
    for (auto& material : mMaterials) {
        if (material.mCount > 0)
            mDatasmithScene->AddMaterial(material.mMaterialElement);
    }
}

// For Datasmith::FSceneValidator::PrintReports
static void Trace(const utf8_t* FormatString, ...) {
    va_list argptr;
    va_start(argptr, FormatString);
    vfprintf(stderr, FormatString, argptr);
    va_end(argptr);
}

// Convert Vim scene to a datasmith scene
void CVimToDatasmith::CreateDatasmithScene() {
    TraceF("CVimToDatasmith::CreateDatasmithScene\n");
    mDatasmithScene = FDatasmithSceneFactory::CreateScene(UTF8_TO_TCHAR(mVimFilePath.c_str()));

    mDatasmithScene->SetHost(TEXT("Vim"));
    mDatasmithScene->SetVendor(TEXT("VIMaec"));
    mDatasmithScene->SetProductName(TEXT("VimToDatasmith"));
    mDatasmithScene->SetProductVersion(UTF8_TO_TCHAR("1.0.0"));

    CreateMaterials();
    ProcessGeometry();

    mPrepareTimeStat.ReStart();

    PrintStats();

    try {
        ProcessDefinitions();
        ProcessInstances();
    } catch (...) {
        CTaskMgr::DeleteMgr(); // Kill all tasks
        throw;
    }
    CTaskMgr::GetMgr()->Join();

    CreateActors();
    AddUsedMaterials();
    mConvertTimeStat.ReStart();

#if defined(DEBUG)
    Datasmith::FSceneValidator validator(mDatasmithScene.ToSharedRef());
    validator.CheckElementsName();
    validator.CheckDependances();
    validator.PrintReports(Datasmith::FSceneValidator::kVerbose, Trace);
#endif

    mValidationTimeStat.ReStart();
}

// Write a Datasmith scene to the Datasmith file
void CVimToDatasmith::CreateDatasmithFile() {
    TraceF("CVimToDatasmith::CreateDatasmithFile\n");

    FDatasmithSceneExporter SceneExporter;
    SceneExporter.PreExport();
    SceneExporter.SetName(UTF8_TO_TCHAR(mDatasmithFileName.c_str()));

    SceneExporter.SetOutputPath(UTF8_TO_TCHAR(mDatasmithFolderPath.c_str()));

    SceneExporter.Export(mDatasmithScene.ToSharedRef());
    mWriteTimeStat.ReStart();
    ReportTimeStat();
}

// Print selected contents
void CVimToDatasmith::PrintStats() {
    TraceF("Positions %lu, Indices=%lu, Groups=%lu, Nodes=%lu, Strings=%lu\n", mPositions.Count(), mIndices.Count(), mGroupVertexOffets.Count(),
           mVimScene.mNodes.size(), mVimScene.mStrings.size());

#if 0
#if 0
	TraceF("Nodes count %lu\n", mVimScene.mNodes.size());
	size_t validNodesCount = 0;
	size_t validParentsCount = 0;
	size_t validGeometriesCount = 0;
	size_t validInstancesCount = 0;
	for (size_t index = 0; index < mVimScene.mNodes.size(); ++index) {
		const auto& node = mVimScene.mNodes[index];
		if (node.mParent != kNoParent || node.mGeometry != kNoGeometry || node.mInstance != kNoInstance) {
			if (node.mParent != kNoParent)
				++validParentsCount;
			if (node.mGeometry != kNoGeometry)
				++validGeometriesCount;
			if (node.mInstance != kNoInstance)
				++validInstancesCount;
			++validNodesCount;
			const cMat4& nodeTrans = ToMat4(node.mTransform);
			TraceF("\tNode[%lu] Parent=%d, Geometry=%d, Instance=%d Transform={%s}\n", index, node.mParent, node.mGeometry, node.mInstance, ToString(nodeTrans, " ").c_str());
		}
	}
	TraceF("Nodes count %lu - Parent=%lu, Geometries=%lu, Instances=%lu\n", validNodesCount, validParentsCount, validGeometriesCount, validInstancesCount);
#endif
#if 0
	TraceF("String count %lu\n", mVimScene.mStrings.size());
	for (size_t index = 0; index < mVimScene.mStrings.size(); ++index)
		TraceF("\t%lu \"%s\"\n", index, mVimScene.mStrings[index]);
#endif

#if 1
	TraceF("Entities count= %ld\n", mVimScene.mEntityTables.size());
	for (const auto& entity : mVimScene.mEntityTables)
		TraceF("\t%s\n", entity.first.c_str());
	
	TraceF("Assets count= %ld\n", mVimScene.mAssetsBFast.buffers.size());
	for (const auto& asset : mVimScene.mAssetsBFast.buffers)
		TraceF("\t%s\n", asset.name.c_str());

	mPositions.Print("Positions");
	mIndices.Print("Indices");
	mMaterialIds.Print("MaterialIds");
	mObjectIds.Print("ObjectIdsX");
	mGroupIndexOffets.Print("GroupIndexOffets");
	mGroupVertexOffets.Print("GroupVertexOffets");

	mGroupIndexCounts.Print("GroupIndexCounts");
#endif
#endif
}

// Print time statistics
void CVimToDatasmith::ReportTimeStat() {
    mWriteTimeStat.PrintDiff("Total", mStartTimeStat);
    mSetupTimeStat.PrintDiff("Setup", mStartTimeStat);
    mReadTimeStat.PrintDiff("Read", mSetupTimeStat);
    mPrepareTimeStat.PrintDiff("Prepare", mReadTimeStat);
    mConvertTimeStat.PrintDiff("Convert", mPrepareTimeStat);
    mValidationTimeStat.PrintDiff("Validation", mConvertTimeStat);
    mWriteTimeStat.PrintDiff("Write", mValidationTimeStat);
}

FTransform ToFTransform(const cMat4& inMat) {
    cQuat quat;
    quat.FromMat4(inMat);
    quat.Normalise();

    FQuat rotation(-quat.mQuat.x, quat.mQuat.y, -quat.mQuat.z, -quat.mQuat.w);
    FVector translation(inMat.m30 * Meter2Centimeter, -inMat.m31 * Meter2Centimeter, inMat.m32 * Meter2Centimeter);
    FVector scale(inMat.mRow0.Length(), inMat.mRow1.Length(), inMat.mRow2.Length());

    return FTransform(rotation, translation, scale);
}

// Return the material name
const TCHAR* CVimToDatasmith::GetMaterialName(uint32_t inVimMaterialId) const {
    auto iterFound = mVimToDatasmithMaterialMap.find(inVimMaterialId);
    TestAssert(iterFound != mVimToDatasmithMaterialMap.end() && iterFound->second < mMaterials.size());
    const MaterialEntry& materialEntry = mMaterials[iterFound->second];
    return materialEntry.mMaterialElement->GetName();
}

// Compute the hash of the materials used
CMD5Hash CVimToDatasmith::ComputeHash(const MapVimMaterialIdToDsMeshMaterialIndice& inVimMaterialIdToDsMeshMaterialIndice) const {
    FMD5 MD5;
    for (auto& iter : inVimMaterialIdToDsMeshMaterialIndice) {
        const TCHAR* materialName = GetMaterialName(iter.first);
        MD5.Update(reinterpret_cast<const uint8*>(materialName), FCString::Strlen(materialName) * sizeof(TCHAR));
        MD5.Update(reinterpret_cast<const uint8*>(&iter.second), sizeof(iter.second));
    }
    return CMD5Hash(&MD5);
}

std::mutex sAccessControl;

CVimToDatasmith::CMeshDefinition::CMeshDefinition() {
}

CVimToDatasmith::CMeshElement* CVimToDatasmith::CMeshDefinition::Initialize(FDatasmithMesh& inMesh,
                                                                            const MapVimMaterialIdToDsMeshMaterialIndice& inVimMaterialIdToDsMeshMaterialIndice,
                                                                            CVimToDatasmith& inVimToDatasmith) {
    TCHAR SubDir1[2] = {inMesh.GetName()[0], 0};
    TCHAR SubDir2[2] = {inMesh.GetName()[1], 0};
    FString OutputPath(FPaths::Combine(inVimToDatasmith.mOutputPath, SubDir1, SubDir2));

    // Create a new mesh file
    FDatasmithMeshExporter MeshExporter;
    TSharedPtr<IDatasmithMeshElement> meshElement = MeshExporter.ExportToUObject(*OutputPath, inMesh.GetName(), inMesh, nullptr, EDSExportLightmapUV::Never);
    if (meshElement.IsValid()) {
        // meshElement->SetLabel(UTF8_TO_TCHAR(Utf8StringFormat("Geometry %d", geometryIndex).c_str()));
        for (auto& iter : inVimMaterialIdToDsMeshMaterialIndice) {
            size_t materialIndex = inVimToDatasmith.mVimToDatasmithMaterialMap[iter.first];
            TestAssert(materialIndex < inVimToDatasmith.mMaterials.size());
            MaterialEntry& materialEntry = inVimToDatasmith.mMaterials[materialIndex];
            materialEntry.mCount++;
            meshElement->SetMaterial(materialEntry.mMaterialElement->GetName(), iter.second);
        }
        {
            std::lock_guard<std::mutex> lock(sAccessControl);
            inVimToDatasmith.mDatasmithScene->AddMesh(meshElement);
        }
    }

    mFirstElement = GetOrCreateMeshElement(inVimMaterialIdToDsMeshMaterialIndice, inVimToDatasmith);
    mFirstElement->InitAsFirstElement(meshElement);
    return mFirstElement;
}

// Return a mesh element for the material list specified.
CVimToDatasmith::CMeshElement*
CVimToDatasmith::CMeshDefinition::GetOrCreateMeshElement(const MapVimMaterialIdToDsMeshMaterialIndice& inVimMaterialIdToDsMeshMaterialIndice,
                                                         CVimToDatasmith& inVimToDatasmith) {
    CMD5Hash MD5Hash(inVimToDatasmith.ComputeHash(inVimMaterialIdToDsMeshMaterialIndice));

    std::lock_guard<std::mutex> lock(sAccessControl);
    auto insertResult = mMapMaterialMD5ToMeshElement.insert({MD5Hash, std::unique_ptr<CMeshElement>()});
    if (insertResult.second)
        insertResult.first->second.reset(new CMeshElement(*this, inVimMaterialIdToDsMeshMaterialIndice, MD5Hash));
    return insertResult.first->second.get();
}

CVimToDatasmith::CMeshElement::CMeshElement(const CMeshDefinition& inMeshDefinition,
                                            const MapVimMaterialIdToDsMeshMaterialIndice& inVimMaterialIdToDsMeshMaterialIndice,
                                            const CMD5Hash& inMaterialsMD5Hash)
: mMeshDefinition(inMeshDefinition)
, mVimMaterialIdToDsMeshMaterialIndice(inVimMaterialIdToDsMeshMaterialIndice)
, mMaterialsMD5Hash(inMaterialsMD5Hash) {
}

void CVimToDatasmith::CMeshElement::InitAsFirstElement(const TSharedPtr<IDatasmithMeshElement>& inFirstElement) {
    std::lock_guard<std::mutex> lock(sAccessControl);
    mMeshElement = inFirstElement;
}

void CVimToDatasmith::CMeshElement::InitWithFirstElement(const CMeshElement& inFirstElement) {
    TestAssert(inFirstElement.mMeshElement.IsValid() && !mMeshElement.IsValid());
    const IDatasmithMeshElement& firstElement = *inFirstElement.mMeshElement;
    CMD5Hash myHash = CMD5Hash(firstElement.GetFileHash()).CombineWith(mMaterialsMD5Hash);
    mMeshElement = FDatasmithSceneFactory::CreateMesh(*myHash.ToString());
    IDatasmithMeshElement& meshElement = *mMeshElement;
    meshElement.SetFile(firstElement.GetFile());
    meshElement.SetFileHash(firstElement.GetFileHash());
    meshElement.SetDimensions(firstElement.GetArea(), firstElement.GetWidth(), firstElement.GetHeight(), firstElement.GetDepth());
    meshElement.SetLightmapCoordinateIndex(firstElement.GetLightmapCoordinateIndex());
    meshElement.SetLightmapSourceUV(firstElement.GetLightmapSourceUV());

#if 0 // Just as sample code, it's not needed in our case: see InitMeshMaterials
	int32 slotCount = firstElement.GetMaterialSlotCount();
	for (int i = 0; i < slotCount; ++i) {
		TSharedPtr<const IDatasmithMaterialIDElement> materialSlot = firstElement.GetMaterialSlotAt(i);
		if (materialSlot.IsValid())
			meshElement.SetMaterial(materialSlot->GetName(), materialSlot->GetId());
	}
#endif
}

void CVimToDatasmith::CMeshElement::InitMeshMaterials(const CVimToDatasmith& inVimToDatasmith) {
    for (auto& iter : mVimMaterialIdToDsMeshMaterialIndice) {
        mMeshElement->SetMaterial(inVimToDatasmith.GetMaterialName(iter.first), iter.second);
    }
}

// Constructor
CVimToDatasmith::CGeometryEntry::CGeometryEntry(CVimToDatasmith* inVimToDatasmith, const Vim::SceneNode& inDefinition)
: mVimToDatasmith(inVimToDatasmith)
, mDefinition(inDefinition) {
    CTaskMgr::GetMgr()->AddTask(this);
}

void CVimToDatasmith::CGeometryEntry::Run() {
    int32_t geometryIndex = mDefinition.mGeometry;
    if (geometryIndex == kNoGeometry)
        geometryIndex = mVimToDatasmith->mVimScene.mNodes[mDefinition.mInstance].mGeometry;

    if (ToMat4(mDefinition).IsIdentity()) {
    }

    FDatasmithMesh datasmithMesh;
    MapVimMaterialIdToDsMeshMaterialIndice vimMaterialIdToDsMeshMaterialIndice;
    mVimToDatasmith->ConvertGeometryToDatasmithMesh(geometryIndex, &datasmithMesh, &vimMaterialIdToDsMeshMaterialIndice);

    if (!vimMaterialIdToDsMeshMaterialIndice.empty()) {
        Datasmith::FDatasmithHash meshHasher;
        meshHasher.ComputeDatasmithMeshHash(datasmithMesh);
        FMD5Hash meshHash = meshHasher.GetHashValue();
        CMD5Hash meshMD5Hash(meshHash);

        // List of already created mesh assets (the key is the MD5Hash of the mesh definition)
        bool isNewDefinition = false;
        CMeshDefinition* meshDefinition = nullptr;
        {
            std::unique_lock<std::mutex> lk(mVimToDatasmith->mDefinitionsAccessControl);
            auto insertResult = mVimToDatasmith->mMeshDefinitions.insert({meshMD5Hash, std::unique_ptr<CMeshDefinition>()});
            if (insertResult.second) {
                insertResult.first->second.reset(new CMeshDefinition());
                isNewDefinition = true;
            }
            meshDefinition = insertResult.first->second.get();
        }
        if (isNewDefinition) {
            datasmithMesh.SetName(*LexToString(meshHash));
            mMeshElement = meshDefinition->Initialize(datasmithMesh, vimMaterialIdToDsMeshMaterialIndice, *mVimToDatasmith);
        } else
            mMeshElement = meshDefinition->GetOrCreateMeshElement(vimMaterialIdToDsMeshMaterialIndice, *mVimToDatasmith);
    }
}

void CVimToDatasmith::CGeometryEntry::AddInstance(const Vim::SceneNode* inInstance) {
    if (mInstances == nullptr)
        mInstances.reset(new std::vector<const Vim::SceneNode*>{inInstance});
    else
        mInstances->push_back(inInstance);
}

FString CVimToDatasmith::CGeometryEntry::HashToName(Datasmith::FDatasmithHash& hasher, const FString& inLabel) const {
    // Hash mesh name
    const IDatasmithMeshElement* meshElement = mMeshElement->GetMeshElement(*mVimToDatasmith);
    hasher.MyMD5.Update((const unsigned char*)meshElement->GetName(), FCString::Strlen(meshElement->GetName()) * sizeof(TCHAR));

    FMD5Hash actorHash = hasher.GetHashValue();
    FString actorName(LexToString(actorHash));

    // Validate actor name unicity (or make-it)
    utf8_string tmpName(TCHAR_TO_UTF8(*actorName));
    auto insertResult = mVimToDatasmith->mMapInstancesNameToGeometry.insert({tmpName, this});
    if (!insertResult.second) {
        uint32_t occurence = 1;
        do {
            utf8_string occurenceName = Utf8StringFormat("%s_Occurence_%d", tmpName.c_str(), ++occurence);
            insertResult = mVimToDatasmith->mMapInstancesNameToGeometry.insert({occurenceName, this});
        } while (!insertResult.second);
        actorName = UTF8_TO_TCHAR(insertResult.first->first.c_str());
        TraceF("Duplicate actor name %s - label %s\n", insertResult.first->first.c_str(), TCHAR_TO_UTF8(*inLabel));
    }

    return actorName;
}

void CVimToDatasmith::CGeometryEntry::CreateActor(const Vim::SceneNode& inSceneNode) {
    FTransform actorTransfo(ToFTransform(ToMat4(inSceneNode)));

    size_t indexNode = &inSceneNode - &mVimToDatasmith->mVimScene.mNodes[0];
    FString label(*FString::Printf(TEXT("Node_%lu"), indexNode));

#if defined(DEBUG) && 0
    if (indexNode == 111) {
        TraceF("indexNode %lu\n", indexNode);
    }

    int32_t geometryIndex = mDefinition->mGeometry;
    if (geometryIndex == kNoGeometry)
        geometryIndex = mVimToDatasmith->mVimScene.mNodes[mDefinition->mInstance].mGeometry;

    if (geometryIndex > 111) {
        TraceF("geometryIndex %d\n", geometryIndex);
    }
#endif

    // Compute the actor name
    Datasmith::FDatasmithHash hasher;
    hasher.HashQuat(actorTransfo.GetRotation());
    hasher.HashFixVector(actorTransfo.GetTranslation());
    hasher.HashScaleVector(actorTransfo.GetScale3D());

    FString actorName(HashToName(hasher, label));

    // Create the actor
    TSharedRef<IDatasmithMeshActorElement> meshActor(FDatasmithSceneFactory::CreateMeshActor(*actorName));

    meshActor->SetLabel(*label);

    // Set the position of this actor
    meshActor->SetRotation(actorTransfo.GetRotation(), false);
    meshActor->SetTranslation(actorTransfo.GetTranslation(), false);
    meshActor->SetScale(actorTransfo.GetScale3D(), false);

    meshActor->SetStaticMeshPathName(mMeshElement->GetMeshElement(*mVimToDatasmith)->GetName());

    // Add the new actor to the scene
    mVimToDatasmith->mDatasmithScene->AddActor(meshActor);
}

void CVimToDatasmith::CGeometryEntry::CreateHierarchicalInstancesActor() {
    size_t indexNode = &mDefinition - &mVimToDatasmith->mVimScene.mNodes[0];
    FString label(*FString::Printf(TEXT("Node_%lu"), indexNode));

    // Compute the actor name
    Datasmith::FDatasmithHash hasher;

    // Hash 1st instance (definition) transformation
    FTransform definitionTransfo = ToFTransform(ToMat4(mDefinition));
    hasher.HashQuat(definitionTransfo.GetRotation());
    hasher.HashFixVector(definitionTransfo.GetTranslation());
    hasher.HashScaleVector(definitionTransfo.GetScale3D());

    // Hash all instances
    for (auto& instance : *mInstances) {
        FTransform instanceTransfo = ToFTransform(ToMat4(*instance));
        hasher.HashQuat(instanceTransfo.GetRotation());
        hasher.HashFixVector(instanceTransfo.GetTranslation());
        hasher.HashScaleVector(instanceTransfo.GetScale3D());
    }

    FString actorName(HashToName(hasher, label));

    // Create the actor
    auto hierarchicalMeshActor(FDatasmithSceneFactory::CreateHierarchicalInstanceStaticMeshActor(*actorName));

    hierarchicalMeshActor->SetLabel(*label);

    hierarchicalMeshActor->ReserveSpaceForInstances(int32(mInstances->size() + 1));

    hierarchicalMeshActor->AddInstance(definitionTransfo);
    for (auto& instance : *mInstances) {
        FTransform instanceTransfo(ToFTransform(ToMat4(*instance)));
        hierarchicalMeshActor->AddInstance(instanceTransfo);
    }

    hierarchicalMeshActor->SetStaticMeshPathName(mMeshElement->GetMeshElement(*mVimToDatasmith)->GetName());

    // Add the new actor to the scene
    mVimToDatasmith->mDatasmithScene->AddActor(hierarchicalMeshActor);
}

void CVimToDatasmith::CGeometryEntry::CreateActors() {
    const IDatasmithMeshElement* meshElement = mMeshElement != nullptr ? mMeshElement->GetMeshElement(*mVimToDatasmith) : nullptr;
    if (meshElement != nullptr) {
        if (mInstances == nullptr)
            CreateActor(mDefinition);
        else if (mVimToDatasmith->mNoHierarchicalInstance) {
            CreateActor(mDefinition);
            for (auto& instance : *mInstances)
                CreateActor(*instance);
        } else
            CreateHierarchicalInstancesActor();
    }
}

} // namespace Vim2Ds
