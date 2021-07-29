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

#include "Math/Transform.h"
#include "Paths.h"

DISABLE_SDK_WARNINGS_END

#if defined(DEBUG) && 1
#define UseValidator 1
#else
#define UseValidator 0
#endif

#if UseValidator
#include "DatasmithSceneValidator.h"
#endif

#include <iostream>

extern "C" {
bool CreateDirectoryW(wchar_t* lpPathName, void* lpSecurityAttributes);
}

namespace Vim2Ds {

// Constructor
CVimToDatasmith::MaterialEntry::MaterialEntry(MaterialId inVimId)
: mCount(0)
, mVimId(inVimId)
, mMaterialElement(FDatasmithSceneFactory::CreateUEPbrMaterial(UTF8_TO_TCHAR(Utf8StringFormat("%u", inVimId).c_str()))) {
}

// Copy constructor required to be in a std::vector
CVimToDatasmith::MaterialEntry::MaterialEntry(const MaterialEntry& inOther)
: mCount((int32_t)inOther.mCount)
, mVimId(inOther.mVimId)
, mMaterialElement(inOther.mMaterialElement)
, mColor(inOther.mColor)
, mParams(inOther.mParams)
, mTexture(inOther.mTexture) {
}

// Constructor
CVimToDatasmith::CVimToDatasmith() {
    mStartTimeStat.BeginNow();
    mGeometryToDatasmithMeshMap[GeometryIndex::kNoGeometry] = TSharedPtr<IDatasmithMeshElement>();
}

IDatasmithMetaDataElement& CVimToDatasmith::CActorEntry::GetOrCreateMetadataElement(CVimToDatasmith* inVimToDatasmith) {
    if (!mMetaDataElement.IsValid()) {
        TestPtr(mActorElement);
        FString metadataName(FString::Printf(TEXT("MetaData_%s"), mActorElement->GetName()));
        mMetaDataElement = FDatasmithSceneFactory::CreateMetaData(*metadataName);
        mMetaDataElement->SetAssociatedElement(mActorElement);
        inVimToDatasmith->mDatasmithScene->AddMetaData(mMetaDataElement);
    }
    return *mMetaDataElement;
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
    mReadTimeStat.BeginNow();
    Vim::VimErrorCodes vimReadResult = mVimScene.ReadFile(mVimFilePath);
    if (vimReadResult != Vim::VimErrorCodes::Success)
        ThrowMessage("CVimToDatasmith::ReadVimFile - ReadFile return error %d", vimReadResult);
    mReadTimeStat.FinishNow();
#if 0
    DumpAssets();
    DumpEntitiesTables();
#endif
}

inline void DumpIndexColumn(const utf8_t* inTableName, const utf8_t* inColumnName, const std::vector<int>& inColumn) {
    for (size_t i = 0; i < inColumn.size(); ++i)
        TraceF("\t[\"%s\"][\"%s\"][%lu] %d\n", inTableName, inColumnName, i, inColumn[i]);
}

void CVimToDatasmith::DumpStringColumn(const utf8_t* inTableName, const utf8_t* inColumnName, const std::vector<int>& inColumn) const {
    for (size_t i = 0; i < inColumn.size(); ++i)
        TraceF("\t[\"%s\"][\"%s\"][%lu] \"%s\"\n", inTableName, inColumnName, i, GetVimString(StringIndex(inColumn[i])));
}

inline void DumpNumericColumn(const utf8_t* inTableName, const utf8_t* inColumnName, const std::vector<double>& inColumn) {
    for (size_t i = 0; i < inColumn.size(); ++i)
        TraceF("\t[\"%s\"][\"%s\"][%lu] %lf\n", inTableName, inColumnName, i, inColumn[i]);
}

void CVimToDatasmith::DumpTable(const utf8_t* inMsg, const Vim::EntityTable& inTable, bool inContent) const {
    for (auto iter : inTable.mIndexColumns) {
        TraceF("[\"%s\"] - mIndexColumns[%s] size=%lu\n", inMsg, iter.first.c_str(), iter.second.size());
        if (inContent)
            DumpIndexColumn(inMsg, iter.first.c_str(), iter.second);
    }
    for (auto iter : inTable.mStringColumns) {
        TraceF("[\"%s\"] - mStringColumns[%s] size=%lu\n", inMsg, iter.first.c_str(), iter.second.size());
        if (inContent)
            DumpStringColumn(inMsg, iter.first.c_str(), iter.second);
    }
    for (auto iter : inTable.mNumericColumns) {
        TraceF("[\"%s\"] - mNumericColumns[%s] size=%lu\n", inMsg, iter.first.c_str(), iter.second.size());
        if (inContent)
            DumpNumericColumn(inMsg, iter.first.c_str(), iter.second);
    }
    int previousEntityId = -1;
    for (auto& property : inTable.mProperties) {
        if (previousEntityId != property.mEntityId)
            TraceF("[\"%s\"] - Property id=%d\n", inMsg, property.mEntityId);
        TraceF("\t\t{ Name=\"%s\" Value=\"%s\" }\n", GetVimString(StringIndex(property.mName)), GetVimString(StringIndex(property.mValue)));
        previousEntityId = property.mEntityId;
    }
    TraceF("\n");
}

void CVimToDatasmith::DumpAssets() const {
    TraceF("Assets count= %ld\n", mVimScene.mAssetsBFast.buffers.size());
    for (const auto& asset : mVimScene.mAssetsBFast.buffers)
        TraceF("\t%s\n", asset.name.c_str());
}

void CVimToDatasmith::DumpEntitiesTables() const {
    for (auto table : mVimScene.mEntityTables)
        DumpTable(table.first.c_str(), table.second, true);
}

CVimToDatasmith::CTextureEntry::CTextureEntry(CVimToDatasmith* inVimToDatasmith, const bfast::Buffer& inImageBuffer)
: mVimToDatasmith(inVimToDatasmith)
, mImageBuffer(inImageBuffer) {
    // Name of texture is the content.
    FMD5 MD5;
    MD5.Update(mImageBuffer.data.begin(), mImageBuffer.data.size());
    CMD5Hash MD5Hash(&MD5);
    mDatasmithName = MD5Hash.ToString();
    mDatasmithLabel = UTF8_TO_TCHAR(mImageBuffer.name.c_str());
}

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

void CVimToDatasmith::CTextureEntry::CopyTextureInAssets() {
    if (!CreateFolder(TCHAR_TO_UTF8(*mVimToDatasmith->mOutputPath)))
        return;
    FString textureFolderPath = mVimToDatasmith->mOutputPath + TEXT("/Textures");
    if (!CreateFolder(TCHAR_TO_UTF8(*textureFolderPath)))
        return;

    FString filePathName = textureFolderPath + TEXT("/") + mDatasmithName;
    size_t posExtension = mImageBuffer.name.find_last_of('.');
    if (posExtension != std::string::npos)
        filePathName += UTF8_TO_TCHAR(mImageBuffer.name.c_str() + posExtension);
    else {
        DebugF("CVimToDatasmith::CTextureEntry::CopyTextureInAssets - Texture file \"%s\" hasn't extension\n", mImageBuffer.name.c_str());
        return;
    }

#ifndef __clang__
    FILE* file = nullptr;
    if (_wfopen_s(&file, *filePathName, TEXT("wb")) != 0)
        file = nullptr;
#else
    FILE* file = fopen(TCHAR_TO_UTF8(*filePathName), "wb");
#endif
    if (file != nullptr) {
        if (fwrite(mImageBuffer.data.begin(), mImageBuffer.data.size(), 1, file) != 1)
            DebugF("CVimToDatasmith::CTextureEntry::CopyTextureInAssets - Can't write to file \"%s\" - error=%d\n", TCHAR_TO_UTF8(*filePathName), errno);
        fclose(file);
    } else
        DebugF("CVimToDatasmith::CTextureEntry::CopyTextureInAssets - Can't open file \"%s\" for writing - error=%d\n", TCHAR_TO_UTF8(*filePathName), errno);
}

void CVimToDatasmith::CTextureEntry::AddToScene() {
    if (!mDatasmithTexture.IsValid()) {
        CopyTextureInAssets();
        mDatasmithTexture = FDatasmithSceneFactory::CreateTexture(*mDatasmithName);
        size_t posExtension = mImageBuffer.name.find_last_of('.');
        const char* extension = ".png";
        EDatasmithTextureFormat fmt = EDatasmithTextureFormat::PNG;
        if (posExtension != std::string::npos) {
            extension = mImageBuffer.name.c_str() + posExtension;
            if (strcmp(extension, ".jpg") == 0)
                fmt = EDatasmithTextureFormat::JPEG;
        }
#if 1
        FString filePathName = mVimToDatasmith->mOutputPath + TEXT("/Textures/") + mDatasmithName;
        filePathName += UTF8_TO_TCHAR(extension);
        mDatasmithTexture->SetFile(*filePathName);
        FMD5Hash FileHash = FMD5Hash::HashFile(mDatasmithTexture->GetFile());
        mDatasmithTexture->SetFileHash(FileHash);
#else
        mDatasmithTexture->SetData(mImageBuffer.data.begin(), uint32(mImageBuffer.data.size()), fmt);
#endif

        mDatasmithTexture->SetLabel(*mDatasmithLabel);
        mDatasmithTexture->SetSRGB(EDatasmithColorSpace::sRGB);
        mVimToDatasmith->mDatasmithScene->AddTexture(mDatasmithTexture);
    }
}

CVimToDatasmith::CTextureEntry* CVimToDatasmith::CreateTexture(const utf8_t* inTextureName) {
    static const utf8_t texturePrefix[] = "textures\\";
    size_t l = strlen(texturePrefix);

    auto insertResult = mVimTextureToTextureMap.insert({inTextureName, std::unique_ptr<CTextureEntry>()});
    if (insertResult.second) {
        for (const auto& asset : mVimScene.mAssetsBFast.buffers) {
            if (asset.name.compare(0, l, texturePrefix) == 0) {
                if (asset.name.compare(l, std::string::npos, inTextureName) == 0) {
                    insertResult.first->second.reset(new CTextureEntry(this, asset));
                    break;
                }
            }
        }
        if (!insertResult.first->second)
            DebugF("CVimToDatasmith::CreateTexture - Texture \"%s\" not found\n", inTextureName);
    }

    return insertResult.first->second.get();
}

// Create datasmith materials from Vim ones
void CVimToDatasmith::CreateMaterials() {
    VerboseF("CVimToDatasmith::CreateMaterials\n");
    Vim::EntityTable& materialTable = mVimScene.mEntityTables["table:Rvt.Material"];

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
        MaterialId vimMaterialId = (MaterialId)idArray[i];

        VerboseF("CVimToDatasmith::CreateMaterials - MaterialId = %u\n", vimMaterialId);

        auto previous = mVimToDatasmithMaterialMap.find(vimMaterialId);
        if (previous != mVimToDatasmithMaterialMap.end()) {
            DebugF("MaterialId %u \"%s\" duplicated Index=%lu vs %lu\n", vimMaterialId, GetVimString(nameArray, i), i, previous->second);
            continue;
        }
        mVimToDatasmithMaterialMap[vimMaterialId] = mMaterials.size();

        CVimToDatasmith::MaterialEntry materialEntry(vimMaterialId);

        materialEntry.mColor.x = colorXArray.size() > i ? (float)colorXArray[i] : 1.0f;
        materialEntry.mColor.y = colorYArray.size() > i ? (float)colorYArray[i] : 1.0f;
        materialEntry.mColor.z = colorZArray.size() > i ? (float)colorZArray[i] : 1.0f;
        materialEntry.mColor.w = transparencyArray.size() > i ? 1.0f - (float)transparencyArray[i] : 1.0f;

        materialEntry.mParams.x = glossinessArray.size() > i ? (float)glossinessArray[i] / 256.0f : 0.5f;
        materialEntry.mParams.y = smoothnessArray.size() > i ? (float)smoothnessArray[i] / 256.0f : 50.0f / 256.0f;

        const utf8_t* textureName = GetVimString(textureNameArray, i);
        if (*textureName)
            materialEntry.mTexture = CreateTexture(textureName);

        IDatasmithUEPbrMaterialElement& element = materialEntry.mMaterialElement.Get();
        const utf8_t* materialName = GetVimString(nameArray, i);
        element.SetLabel(UTF8_TO_TCHAR((*materialName ? materialName : Utf8StringFormat("Vim %d", vimMaterialId).c_str())));

        if (materialEntry.mTexture != nullptr) {
            IDatasmithMaterialExpressionTexture* baseTextureExpression = element.AddMaterialExpression<IDatasmithMaterialExpressionTexture>();
            baseTextureExpression->SetTexturePathName(materialEntry.mTexture->GetName());
            baseTextureExpression->SetName(materialEntry.mTexture->GetLabel());
            baseTextureExpression->ConnectExpression(element.GetBaseColor());
        } else {
            IDatasmithMaterialExpressionColor* DiffuseExpression = element.AddMaterialExpression<IDatasmithMaterialExpressionColor>();
            if (DiffuseExpression != nullptr) {
                FLinearColor dsColor(materialEntry.mColor.x, materialEntry.mColor.y, materialEntry.mColor.z, materialEntry.mColor.w);
                DiffuseExpression->GetColor() = dsColor;
                DiffuseExpression->SetName(TEXT("Base Color"));
                DiffuseExpression->ConnectExpression(element.GetBaseColor());
                if (materialEntry.mColor.w < 0.999)
                    DiffuseExpression->ConnectExpression(element.GetOpacity(), 3);
            }
        }
        // material.mColor.w = transparencyArray.size() > i ? 1.0f -
        // (float)transparencyArray[i] : 1.0f; material.mParams.x =
        // glossinessArray.size() > i ? (float)glossinessArray[i] / 256.0f :
        // 0.5f; material.mParams.y = smoothnessArray.size() > i ?
        // (float)smoothnessArray[i] / 256.0f : 50.0f / 256.0f;

        mMaterials.push_back(materialEntry);
    }
}

void CVimToDatasmith::CreateAllMetaDatas() {
    mCreateMetaDataStat.BeginNow();
    Vim::EntityTable& elementTable = mVimScene.mEntityTables["table:Rvt.Element"];
    for (auto& property : elementTable.mProperties) {
        ElementIndex elementIndex = ElementIndex(property.mEntityId);
        if (elementIndex != ElementIndex::kNoElement || elementIndex < mVecElementToActors.size()) {
            CActorEntry& actorEntry = mVecElementToActors[elementIndex];
            if (actorEntry.HasElement()) {
                TSharedPtr<IDatasmithKeyValueProperty> dsProperty =
                    FDatasmithSceneFactory::CreateKeyValueProperty(UTF8_TO_TCHAR(GetVimString(StringIndex(property.mName))));
                dsProperty->SetValue(UTF8_TO_TCHAR(GetVimString(StringIndex(property.mValue))));
                dsProperty->SetPropertyType(EDatasmithKeyValuePropertyType::String);
                actorEntry.GetOrCreateMetadataElement(this).AddProperty(dsProperty);
            }
        } else
            TraceF("CVimToDatasmith::CreateAllMetaDatas - Invalid element index %u\n", elementIndex);
    }
    mCreateMetaDataStat.FinishNow();
}

void CVimToDatasmith::CreateAllTags() {
    mCreateTagsStat.BeginNow();
    Vim::EntityTable& elementTable = mVimScene.mEntityTables["table:Rvt.Element"];
    std::vector<int>& elementToLevel = elementTable.mIndexColumns["Level:Level"];
    std::vector<int>& elementToCategory = elementTable.mIndexColumns["Category:Category"];
    std::vector<int>& elementToRoom = elementTable.mIndexColumns["Room:Room"];
    std::vector<int>& elementToFamilyName = elementTable.mStringColumns["FamilyName"];
    std::vector<int>& elementToType = elementTable.mStringColumns["Type"];
    std::vector<double>& elementToId = elementTable.mNumericColumns["Id"];

    for (ElementIndex elementIndex = ElementIndex(0); elementIndex < ElementIndex(mVecElementToActors.size()); Increment(elementIndex)) {
        IDatasmithActorElement* actor = mVecElementToActors[elementIndex].GetActorElement();
        if (actor != nullptr) {
            if (elementIndex < elementToId.size())
                actor->AddTag(*FString::Printf(TEXT("VIM.Id.%lld"), (long long)elementToId[elementIndex]));
            if (elementIndex < elementToLevel.size()) {
                int level = elementToLevel[elementIndex];
                if (level != -1)
                    actor->AddTag(*FString::Printf(TEXT("VIM.Category.%d"), level));
            }
            if (elementIndex < elementToCategory.size()) {
                int category = elementToCategory[elementIndex];
                if (category != -1)
                    actor->AddTag(*FString::Printf(TEXT("VIM.Category.%d"), category));
            }
            if (elementIndex < elementToRoom.size()) {
                int room = elementToRoom[elementIndex];
                if (room != -1)
                    actor->AddTag(*FString::Printf(TEXT("VIM.Room.%d"), room));
            }
            if (elementIndex < elementToFamilyName.size()) {
                StringIndex familyName = StringIndex(elementToFamilyName[elementIndex]);
                if (familyName != -1)
                    actor->AddTag(*FString::Printf(TEXT("VIM.Family.%s"), UTF8_TO_TCHAR(GetVimString(familyName))));
            }
            if (elementIndex < elementToType.size()) {
                StringIndex typeString = StringIndex(elementToType[elementIndex]);
                if (typeString != -1)
                    actor->AddTag(*FString::Printf(TEXT("VIM.Type.%s"), UTF8_TO_TCHAR(GetVimString(typeString))));
            }
        }
    }
    mCreateTagsStat.FinishNow();
}

// Convert geometry to Datasmith Mesh
void CVimToDatasmith::ConvertGeometryToDatasmithMesh(GeometryIndex geometryIndex, FDatasmithMesh* outMesh,
                                                     MapVimMaterialIdToDsMeshMaterialIndice* outVimMaterialIdToDsMeshMaterialIndice) {
    outMesh->SetName(UTF8_TO_TCHAR(Utf8StringFormat("%d", geometryIndex).c_str()));

    // Collect vertex used by this geometry
    std::unordered_map<VertexIndex, int32_t> vimIndiceToDsMeshIndice;
    int32_t verticesCount = 0;
    IndiceIndex indicesStart = mGroupIndexOffets[geometryIndex];
    IndiceIndex indicesEnd = IndiceIndex(indicesStart + mGroupIndexCounts[geometryIndex]);
    for (IndiceIndex index = indicesStart; index < indicesEnd; index = IndiceIndex(index + 1)) {
        VertexIndex vertexIndex = mIndices[index];
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
    FaceIndex vimMaterial = FaceIndex(indicesStart / 3);

    // Copy faces used by this geometry
    int32_t facesCount = mGroupIndexCounts[geometryIndex] / 3;
    TestAssert(facesCount * 3 == mGroupIndexCounts[geometryIndex]);
    outMesh->SetFacesCount(facesCount);
    IndiceIndex vimIndice = indicesStart;
#define ReportInvalid 0
#if ReportInvalid
    bool invalidReported = false;
    static uint32_t sReportedCount = 0;
#endif
    for (int32_t indexFace = 0; indexFace < facesCount; ++indexFace) {
        // Get material
        MaterialId vimMaterialId = mMaterialIds[vimMaterial];
#if ReportInvalid
        if (vimMaterialId == kInvalidMaterial)
            if (invalidReported == false && sReportedCount < 10) {
                invalidReported = true;
                DebugF("CVimToDatasmith::ConvertGeometryToDatasmithMesh - Geometry[%u], Invalid face material [%d]\n", geometryIndex, indexFace);
                if (++sReportedCount == 10)
                    DebugF("CVimToDatasmith::ConvertGeometryToDatasmithMesh - Report limit of 10 reached\n");
            }
#endif
        auto insertResult = outVimMaterialIdToDsMeshMaterialIndice->insert({vimMaterialId, materialsCount});
        vimMaterial = FaceIndex(vimMaterial + 1);
        if (insertResult.second)
            ++materialsCount;

        // Get the face local vertices index.
        int32_t triangleVertices[3];
        cVec3 normals[3];
        for (int i = 0; i < 3; ++i) {
            VertexIndex indice = mIndices[vimIndice];
            vimIndice = IndiceIndex(vimIndice + 1);
            triangleVertices[i] = vimIndiceToDsMeshIndice[indice];
            const cVec3& normal = mNormals[indice];
            outMesh->SetNormal(indexFace * 3 + i, normal.x, -normal.y, normal.z);
        }

        outMesh->SetFace(indexFace, triangleVertices[0], triangleVertices[1], triangleVertices[2], insertResult.first->second);

        /*
        int32_t triangleUVs[3];
        ...
        outMesh->SetFaceUV(indexFace, UVChannel, triangleUVs[0], triangleUVs[1], triangleUVs[2]);
        */
    }
}

// In old vim files, the geometry is exported in world space, even when
// instanced, so we need to remove that world transform from the geometry
void CVimToDatasmith::FixOldVimFileTransforms() {
    if (mVimScene.mVersionMajor == 0 && mVimScene.mVersionMinor == 0 && mVimScene.mVersionPatch <= 200) {
        std::vector<bool> isTransformed(mGroupVertexOffets.Count(), false);

        for (NodeIndex nodeIndex = NodeIndex(0); nodeIndex < mInstancesSubgeometry->Count(); nodeIndex = NodeIndex(nodeIndex + 1)) {
            GeometryIndex subgeometry = (*mInstancesSubgeometry)[nodeIndex];
            if (subgeometry != kNoGeometry) {
                TestAssert(subgeometry < (int)isTransformed.size());
                if (!isTransformed[subgeometry]) {
                    isTransformed[subgeometry] = true;

                    cMat4 invTrans = (*mInstancesTransform)[nodeIndex].Inverse();

                    VertexIndex nextOffset = subgeometry < mGroupVertexOffets.Count() ? mGroupVertexOffets[GeometryIndex(subgeometry + 1)] : mPositions.Count();
                    for (VertexIndex index = mGroupVertexOffets[subgeometry]; index < nextOffset; index = VertexIndex(index + 1)) {
                        cVec3& vertex = mPositions[index];
                        vertex = vertex * invTrans;
                    }
                }
            }
        }
    }
}

void CVimToDatasmith::CollectAttributes() {
    TAttributeVector<cVec4> colorAttribute;
    VerboseF("CVimToDatasmith::CollectAttributes - Begin\n");

    constexpr const char* ObsoleteFaceGroupId = "g3d:face:groupid:0:int32:1";
    constexpr const char* ObsoleteGroupIndexOffset = "g3d:group:indexoffset:0:int32:1";
    constexpr const char* ObsoleteGroupVertexOffset = "g3d:group:vertexoffset:0:int32:1";

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
        else if (descriptorString == g3d::descriptors::FaceGroup || descriptorString == ObsoleteFaceGroupId)
            mObjectIds.Initialize(attr);
        else if (descriptorString == g3d::descriptors::SubgeometryIndexOffset || descriptorString == ObsoleteGroupIndexOffset)
            mGroupIndexOffets.Initialize(attr);
        else if (descriptorString == g3d::descriptors::SubgeometryVertexOffset || descriptorString == ObsoleteGroupVertexOffset)
            mGroupVertexOffets.Initialize(attr);
        else if (descriptorString == g3d::descriptors::InstanceTransform) {
            TestAssert(!mInstancesTransform);
            mInstancesTransform.reset(new TAttributeVector<cMat4, NodeIndex>(attr));
        } else if (descriptorString == g3d::descriptors::InstanceParent) {
            TestAssert(!mInstancesParent);
            mInstancesParent.reset(new TAttributeVector<ParentIndex, NodeIndex>(attr));
        } else if (descriptorString == g3d::descriptors::InstanceSubgeometry) {
            TestAssert(!mInstancesSubgeometry);
            mInstancesSubgeometry.reset(new TAttributeVector<GeometryIndex, NodeIndex>(attr));
        } else if (descriptorString == g3d::descriptors::VertexUv)
            mVertexUVs.Initialize(attr);
        else
            TraceF("Unprocessed attribute \"%s\"\n", descriptorString.c_str());
    }

    TestAssert(mInstancesSubgeometry && mInstancesTransform && mInstancesParent);

    const size_t instancesCount = mInstancesSubgeometry->Count();
    TestAssert(mInstancesTransform->Count() == instancesCount && mInstancesParent->Count() == instancesCount);

    const GeometryIndex groupCount = mGroupIndexOffets.Count();
    TestAssert(groupCount == mGroupVertexOffets.Count());

    // Calculate group index counts
    mGroupIndexCounts.Allocate(groupCount);
    if (groupCount > 0) {
        for (GeometryIndex i = GeometryIndex(0); i < groupCount - 1; i = GeometryIndex(i + 1))
            mGroupIndexCounts[i] = IndiceIndex(mGroupIndexOffets[GeometryIndex(i + 1)] - mGroupIndexOffets[i]);
        mGroupIndexCounts[GeometryIndex(groupCount - 1)] = IndiceIndex(mIndices.Count() - mGroupIndexOffets[GeometryIndex(groupCount - 1)]);
    }

    FixOldVimFileTransforms();

    MeasureTime(ComputeNormals, ComputeNormals(true), kP2DB_Verbose);

    VerboseF("CVimToDatasmith::CollectAttributes - End\n");
}

// Initialize the converter from Vim scene
void CVimToDatasmith::ProcessGeometry() {
    VerboseF("CVimToDatasmith::ProcessGeometry\n");

    CTaskMgr::CTaskJointer jointer("ProcessGeometry");
    CTaskMgr::CTaskJointer convertObsolete("ConvertObsolete");

    (new CTaskMgr::TJoinableFunctorTask<CVimToDatasmith*>([](CVimToDatasmith* inVimToDatasmith) { inVimToDatasmith->ConvertObsoleteSceneNode(); }, this))
        ->Start(&convertObsolete);

    Vim::EntityTable& nodeTable = mVimScene.mEntityTables["table:Vim.Node"];
    std::vector<int>& vimNodeToVimElement = nodeTable.mIndexColumns["Element:Element"];
    mVimNodeToVimElement.Initialize(vimNodeToVimElement);

    Vim::EntityTable& elementTable = mVimScene.mEntityTables["table:Rvt.Element"];
    std::vector<int>& elementToName = elementTable.mStringColumns["Name"];
    mElementToName.Initialize(elementToName);

#if 0
    DumpIndexColumn("table:Vim.Node", "Element:Element", vimNodeToVimElement);
    DumpStringColumn("table:Rvt.Element", "Name", elementToName);
#endif

    (new CTaskMgr::TJoinableFunctorTask<CVimToDatasmith*>(
         [](CVimToDatasmith* inVimToDatasmith) {
             VerboseF("CVimToDatasmith::CollectAttributes - X Begin\n");
             ElementIndex elementsCount = inVimToDatasmith->mElementToName.Count();
             for (ElementIndex elementIndex : inVimToDatasmith->mVimNodeToVimElement)
                 TestAssert(elementIndex == ElementIndex::kNoElement || elementIndex < elementsCount);
             VerboseF("CVimToDatasmith::CollectAttributes - X End\n");
         },
         this))
        ->Start(&jointer);

    convertObsolete.Join();

    (new CTaskMgr::TJoinableFunctorTask<CVimToDatasmith*>([](CVimToDatasmith* inVimToDatasmith) { inVimToDatasmith->CollectAttributes(); }, this))
        ->Start(&jointer);

    jointer.Join();

    TestAssert(mVimNodeToVimElement.Count() == mInstancesSubgeometry->Count());
}

void CVimToDatasmith::ConvertObsoleteSceneNode() {
    VerboseF("CVimToDatasmith::ConvertObsoleteSceneNode - Begin\n");
    for (const auto& buffer : mVimScene.mBfast.buffers) {
        if (buffer.name == "nodes") {
            class CObsoleteSceneNode {
              public:
                ParentIndex mParent;
                GeometryIndex mGeometry;
                int mInstance; // Never used
                float mTransform[16];
            };
            NodeIndex instancesCount = NodeIndex(buffer.data.size() / sizeof(CObsoleteSceneNode));
            TestAssert(instancesCount * sizeof(CObsoleteSceneNode) == buffer.data.size());
            mInstancesTransform.reset(new TAllocatedVector<cMat4, NodeIndex>(instancesCount));
            mInstancesParent.reset(new TAllocatedVector<ParentIndex, NodeIndex>(instancesCount));
            mInstancesSubgeometry.reset(new TAllocatedVector<GeometryIndex, NodeIndex>(instancesCount));

            const CObsoleteSceneNode* nodes = reinterpret_cast<const CObsoleteSceneNode*>(buffer.data.begin());
            for (NodeIndex i = NodeIndex(0); i < instancesCount; i = NodeIndex(i + 1)) {
                (*mInstancesTransform)[i] = *reinterpret_cast<const cMat4*>(nodes->mTransform);
                (*mInstancesParent)[i] = nodes->mParent;
                (*mInstancesSubgeometry)[i] = nodes->mGeometry;
                ++nodes;
            }

            return;
        }
    }
    VerboseF("CVimToDatasmith::ConvertObsoleteSceneNode - End\n");
}

// Datasmith need normals.
void CVimToDatasmith::ComputeNormals(bool inFlip) {
    mNormals.Allocate(mPositions.Count());
    for (IndiceIndex i = IndiceIndex(0); i < mIndices.Count(); i = IndiceIndex(i + 3)) {
        VertexIndex vi0 = mIndices[IndiceIndex(i + 0)];
        VertexIndex vi1 = mIndices[IndiceIndex(i + 1)];
        VertexIndex vi2 = mIndices[IndiceIndex(i + 2)];
        cVec3 v0 = mPositions[vi0];
        cVec3 v1 = mPositions[vi1];
        cVec3 v2 = mPositions[vi2];

        cVec3 s0 = v2 - v0;
        cVec3 s1 = v2 - v1;

        cVec3 normal = s1 ^ s0;
        normal.Normalise();
        if (inFlip)
            normal = -normal;

        mNormals[vi0] += normal;
        mNormals[vi1] += normal;
        mNormals[vi2] += normal;
    }
    for (VertexIndex i = VertexIndex(0); i < mNormals.Count(); i = VertexIndex(i + 1))
        mNormals[i].Normalise();
}

// Create all definitions
void CVimToDatasmith::ProcessInstances() {
    VerboseF("CVimToDatasmith::ProcessDefinitions\n");
    mGeometryEntries.resize(mGroupIndexOffets.Count());
    mVecElementToActors.resize(mElementToName.Count());

    for (NodeIndex nodeIndex = NodeIndex(0); nodeIndex < mInstancesSubgeometry->Count(); nodeIndex = NodeIndex(nodeIndex + 1)) {
        GeometryIndex geometryIndex = (*mInstancesSubgeometry)[nodeIndex];

        // Get instance geometry definition
        if (geometryIndex != kNoGeometry) {
            TestAssert((size_t)geometryIndex < mGeometryEntries.size());
            std::unique_ptr<CGeometryEntry>& geometryEntry = mGeometryEntries[geometryIndex];
            if (geometryEntry == nullptr)
                geometryEntry.reset(new CGeometryEntry(this, geometryIndex, nodeIndex));
            else
                geometryEntry->AddInstance(nodeIndex);
        }
    }
}

// Create all actors
void CVimToDatasmith::CreateActors() {
    VerboseF("CVimToDatasmith::CreateActors\n");

    for (auto& geometry : mGeometryEntries)
        if (geometry != nullptr)
            geometry->CreateActors();
}

// Add Datasmith materials used to the scene
void CVimToDatasmith::AddUsedMaterials() {
    for (auto& material : mMaterials) {
        if (material.mCount > 0) {
            if (material.mTexture != nullptr) {
                material.mTexture->AddToScene();
            }
            mDatasmithScene->AddMaterial(material.mMaterialElement);
        }
    }
}

#if UseValidator
// For Datasmith::FSceneValidator::PrintReports
static void Trace(const utf8_t* FormatString, ...) {
    va_list argptr;
    va_start(argptr, FormatString);
    vfprintf(stderr, FormatString, argptr);
    va_end(argptr);
}
#endif

// Convert Vim scene to a datasmith scene
void CVimToDatasmith::CreateDatasmithScene() {
    VerboseF("CVimToDatasmith::CreateDatasmithScene\n");
    mPrepareTimeStat.BeginNow();
    mDatasmithScene = FDatasmithSceneFactory::CreateScene(UTF8_TO_TCHAR(mVimFilePath.c_str()));

    mDatasmithScene->SetHost(TEXT("Vim"));
    mDatasmithScene->SetVendor(TEXT("VIMaec"));
    mDatasmithScene->SetProductName(TEXT("VimToDatasmith"));
    mDatasmithScene->SetProductVersion(UTF8_TO_TCHAR("1.0.0"));

    CreateMaterials();
    ProcessGeometry();

    mPrepareTimeStat.FinishNow();

    PrintStats();

    mConvertTimeStat.BeginNow();

    try {
        ProcessInstances();
    } catch (...) {
        CTaskMgr::DeleteMgr(); // Kill all tasks
        throw;
    }
    CTaskMgr::Get().Join();

    CreateActors();
    AddUsedMaterials();

    mConvertTimeStat.FinishNow();

    CreateAllMetaDatas();
    CreateAllTags();

#if UseValidator
    mValidationTimeStat.BeginNow();
    Datasmith::FSceneValidator validator(mDatasmithScene.ToSharedRef());
    validator.CheckElementsName();
    validator.CheckDependances();
    validator.PrintReports(Datasmith::FSceneValidator::kVerbose, Trace);
    mValidationTimeStat.FinishNow();
#endif
}

// Write a Datasmith scene to the Datasmith file
void CVimToDatasmith::CreateDatasmithFile() {
    VerboseF("CVimToDatasmith::CreateDatasmithFile\n");
    mWriteTimeStat.BeginNow();

    FDatasmithExportOptions::PathTexturesMode = EDSResizedTexturesPath::OriginalFolder;
    FDatasmithSceneExporter SceneExporter;

    SceneExporter.PreExport();
    SceneExporter.SetName(UTF8_TO_TCHAR(mDatasmithFileName.c_str()));

    SceneExporter.SetOutputPath(UTF8_TO_TCHAR(mDatasmithFolderPath.c_str()));

    SceneExporter.Export(mDatasmithScene.ToSharedRef());
    mWriteTimeStat.FinishNow();

    ReportTimeStat();
}

// Print selected contents
void CVimToDatasmith::PrintStats() {
    VerboseF("Positions %u, Indices=%u, Groups=%u, Instances=%u, Strings=%lu\n", mPositions.Count(), mIndices.Count(), mGroupVertexOffets.Count(),
             mInstancesSubgeometry->Count(), mVimScene.mStrings.size());

#if 0
#if 0
	TraceF("Instances count %u\n", mInstancesSubgeometry->Count());
	size_t validParentsCount = 0;
	size_t validGeometriesCount = 0;
	size_t validTransformCount = 0;
	for (NodeIndex index = kInstanceBegin; index < mInstancesSubgeometry->Count(); index = NodeIndex(index + 1)) {
        GeometryIndex geometry = (*mInstancesSubgeometry)[index];
        int32_t parent = (*mInstancesParent)[index];
        const cMat4& nodeTrans = (*mInstancesTransform)[index];
        if (parent != kNoParent || geometry != kNoGeometry) {
			if (parent != kNoParent)
				++validParentsCount;
			if (geometry != kNoGeometry)
				++validGeometriesCount;
			std::string transfoString("Identity");
            if (!nodeTrans.IsIdentity()) {
                ++validTransformCount;
                transfoString = ToString(nodeTrans, " ");
            }
			TraceF("\tNode[%u] Parent=%d, Geometry=%d, Transform={%s}\n", index, parent, geometry, transfoString.c_str());
		}
	}
	TraceF("Instances - Parent=%lu, Geometries=%lu, Transform=%lu\n", validParentsCount, validGeometriesCount, validTransformCount);
#endif
#if 0
	TraceF("String count %lu\n", mVimScene.mStrings.size());
	for (size_t index = 0; index < mVimScene.mStrings.size(); ++index)
		TraceF("\t%lu \"%s\"\n", index, mVimScene.mStrings[index]);
#endif

#if 0
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
    EP2DB tmp = SetPrintLevel(kP2DB_Trace);
    mStartTimeStat.FinishNow();
    mStartTimeStat.PrintTime("Total", kP2DB_Report);
    mSetupTimeStat.PrintTime("Setup");
    mReadTimeStat.PrintTime("Read");
    mPrepareTimeStat.PrintTime("Prepare");
    mConvertTimeStat.PrintTime("Convert");
    mCreateMetaDataStat.PrintTime("MetaData");
    mCreateTagsStat.PrintTime("Tags");
    mValidationTimeStat.PrintTime("Validation");
    mWriteTimeStat.PrintTime("Write");
    SetPrintLevel(tmp);
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
const TCHAR* CVimToDatasmith::GetMaterialName(MaterialId inVimMaterialId) const {
    auto iterFound = mVimToDatasmithMaterialMap.find(inVimMaterialId);
    size_t materialIndex = 0;
    if (iterFound != mVimToDatasmithMaterialMap.end())
        materialIndex = iterFound->second;
    else if (inVimMaterialId != kInvalidMaterial) {
        static bool sIsReported = false;
        if (sIsReported == false) {
            sIsReported = true;
            DebugF("CVimToDatasmith::GetMaterialName - Unknown material id %u\n", inVimMaterialId);
        }
    }
    TestAssert(materialIndex < mMaterials.size());
    const MaterialEntry& materialEntry = mMaterials[materialIndex];
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
CVimToDatasmith::CGeometryEntry::CGeometryEntry(CVimToDatasmith* inVimToDatasmith, GeometryIndex inGeometry, NodeIndex inDefinition)
: mVimToDatasmith(inVimToDatasmith)
, mGeometry(inGeometry)
, mDefinition(inDefinition) {
    CTaskMgr::Get().AddTask(this);
}

void CVimToDatasmith::CGeometryEntry::Run() {
    if ((*mVimToDatasmith->mInstancesTransform)[mDefinition].IsIdentity()) {
    }

    FDatasmithMesh datasmithMesh;
    MapVimMaterialIdToDsMeshMaterialIndice vimMaterialIdToDsMeshMaterialIndice;
    mVimToDatasmith->ConvertGeometryToDatasmithMesh(mGeometry, &datasmithMesh, &vimMaterialIdToDsMeshMaterialIndice);

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

void CVimToDatasmith::CGeometryEntry::AddInstance(NodeIndex inInstance) {
    if (mInstances == nullptr)
        mInstances.reset(new std::vector<NodeIndex>{inInstance});
    else
        mInstances->push_back(inInstance);
}

FString CVimToDatasmith::CGeometryEntry::HashToName(Datasmith::FDatasmithHash& hasher, NodeIndex inInstance) const {
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
        VerboseF("Duplicate actor name %s - instance %u\n", insertResult.first->first.c_str(), inInstance);
    }

    return actorName;
}

void CVimToDatasmith::CGeometryEntry::AddActor(const TSharedRef<IDatasmithMeshActorElement>& inActor, NodeIndex inInstance) {
    inActor->SetStaticMeshPathName(mMeshElement->GetMeshElement(*mVimToDatasmith)->GetName());

    ElementIndex elementIndex = mVimToDatasmith->mVimNodeToVimElement[inInstance];
    if (elementIndex != ElementIndex::kNoElement) {
        mVimToDatasmith->mVecElementToActors[elementIndex].SetActor(inActor, inInstance);
        inActor->SetLabel(UTF8_TO_TCHAR(mVimToDatasmith->GetVimString(mVimToDatasmith->mElementToName[elementIndex])));
    } else
        DebugF("CVimToDatasmith::CGeometryEntry::CreateActor - Invalid element (instance=%u)\n", inInstance);

    if (!*inActor->GetLabel())
        inActor->SetLabel(*FString::Printf(TEXT("Instance_%u"), inInstance));

    // Add the new actor to the scene
    mVimToDatasmith->mDatasmithScene->AddActor(inActor);
}

void CVimToDatasmith::CGeometryEntry::CreateActor(NodeIndex inInstance) {
    FTransform actorTransfo(ToFTransform((*mVimToDatasmith->mInstancesTransform)[inInstance]));

    // Compute the actor name
    Datasmith::FDatasmithHash hasher;
    hasher.HashQuat(actorTransfo.GetRotation());
    hasher.HashFixVector(actorTransfo.GetTranslation());
    hasher.HashScaleVector(actorTransfo.GetScale3D());

    FString actorName(HashToName(hasher, inInstance));

    // Create the actor
    TSharedRef<IDatasmithMeshActorElement> meshActor(FDatasmithSceneFactory::CreateMeshActor(*actorName));

    // Set the position of this actor
    meshActor->SetRotation(actorTransfo.GetRotation(), false);
    meshActor->SetTranslation(actorTransfo.GetTranslation(), false);
    meshActor->SetScale(actorTransfo.GetScale3D(), false);

    AddActor(meshActor, inInstance);
}

void CVimToDatasmith::CGeometryEntry::CreateHierarchicalInstancesActor() {
    FString label(*FString::Printf(TEXT("Instance_%u"), mDefinition));

    // Compute the actor name
    Datasmith::FDatasmithHash hasher;

    // Hash 1st instance (definition) transformation
    FTransform definitionTransfo(ToFTransform((*mVimToDatasmith->mInstancesTransform)[mDefinition]));
    hasher.HashQuat(definitionTransfo.GetRotation());
    hasher.HashFixVector(definitionTransfo.GetTranslation());
    hasher.HashScaleVector(definitionTransfo.GetScale3D());

    // Hash all instances
    for (NodeIndex instance : *mInstances) {
        FTransform instanceTransfo = ToFTransform((*mVimToDatasmith->mInstancesTransform)[instance]);
        hasher.HashQuat(instanceTransfo.GetRotation());
        hasher.HashFixVector(instanceTransfo.GetTranslation());
        hasher.HashScaleVector(instanceTransfo.GetScale3D());
    }

    FString actorName(HashToName(hasher, mDefinition));

    // Create the actor
    auto hierarchicalMeshActor(FDatasmithSceneFactory::CreateHierarchicalInstanceStaticMeshActor(*actorName));

    hierarchicalMeshActor->ReserveSpaceForInstances(int32(mInstances->size() + 1));

    hierarchicalMeshActor->AddInstance(definitionTransfo);
    for (NodeIndex instance : *mInstances) {
        FTransform instanceTransfo(ToFTransform((*mVimToDatasmith->mInstancesTransform)[instance]));
        hierarchicalMeshActor->AddInstance(instanceTransfo);
    }

    AddActor(hierarchicalMeshActor, mDefinition);
}

void CVimToDatasmith::CGeometryEntry::CreateActors() {
    const IDatasmithMeshElement* meshElement = mMeshElement != nullptr ? mMeshElement->GetMeshElement(*mVimToDatasmith) : nullptr;
    if (meshElement != nullptr) {
        if (mInstances == nullptr)
            CreateActor(mDefinition);
        else if (mVimToDatasmith->mNoHierarchicalInstance) {
            CreateActor(mDefinition);
            for (NodeIndex instance : *mInstances)
                CreateActor(instance);
        } else
            CreateHierarchicalInstancesActor();
    }
}

} // namespace Vim2Ds
