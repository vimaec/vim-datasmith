// Copyright (c) 2021 VIM
// Licensed under the MIT License 1.0

#include "CVimToDatasmith.h"
#include "CConvertVimToDatasmith.h"

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


// Material's collected informations
class CVimToDatasmith::CMaterialEntry {
public:
    // Constructor
    CMaterialEntry(MaterialId inVimId);
    
    // Copy constructor required to be in a std::vector
    CMaterialEntry(const CMaterialEntry& inOther);
    
    std::atomic<int32_t> mCount; // Number of mesh using this materials
    MaterialId mVimId; // Vim material id
    cVec4 mColor; // Vim color
    cVec2 mParams; // Vim glossiness and smoothness
    CTextureEntry* mTexture = nullptr;
    
    TSharedRef<IDatasmithUEPbrMaterialElement> mMaterialElement; // Datasmith version of the materials
};


// Material's collected informations
class CVimToDatasmith::CTextureEntry {
public:
    CTextureEntry(CVimToDatasmith* inVimToDatasmith, const bfast::Buffer& inImageBuffer);
    
    const TCHAR* GetName() const { return *mDatasmithName; }
    
    const TCHAR* GetLabel() const { return *mDatasmithLabel; }
    
    void CopyTextureInAssets();
    
    void AddToScene();
    
private:
    CVimToDatasmith* const mVimToDatasmith;
    const bfast::Buffer& mImageBuffer;
    FString mDatasmithName;
    FString mDatasmithLabel;
    TSharedPtr<IDatasmithTextureElement> mDatasmithTexture;
};


// Material's collected informations
class CVimToDatasmith::CActorEntry {
public:
    CActorEntry() {}
    
    bool HasElement() const { return mActorElement.IsValid(); }
    
    void SetActor(const TSharedRef<IDatasmithActorElement>& inActorElement, NodeIndex inNodeIndex) {
        if (inNodeIndex < mLowestNodeIndex)
            mActorElement = inActorElement;
        TestAssert(mActorElement.IsValid());
    }
    
    IDatasmithActorElement* GetActorElement() { return mActorElement.Get(); }
    
    IDatasmithMetaDataElement& GetOrCreateMetadataElement(CVimToDatasmith* inVimToDatasmith);
    
    void AddTag(const utf8_t* inTag, std::vector<int>& inVector, ElementIndex inIndex);
    
private:
    TSharedPtr<IDatasmithActorElement> mActorElement;
    TSharedPtr<IDatasmithMetaDataElement> mMetaDataElement;
    NodeIndex mLowestNodeIndex = NodeIndex::kNoNode;
};


// MeshDefinition
// name = md5 of mesh data
// Definition is use [1..N] MeshElement
class CVimToDatasmith::CMeshDefinition {
public:
    // Constructor
    CMeshDefinition();
    
    // Initialize the mesh
    CMeshElement* Initialize(FDatasmithMesh& inMesh, const MapVimMaterialIdToDsMeshMaterialIndice& inVimMaterialIdToDsMeshMaterialIndice,
                             CVimToDatasmith& inVimToDatasmith);
    
    // Return a mesh element for the material list specified.
    CMeshElement* GetOrCreateMeshElement(const MapVimMaterialIdToDsMeshMaterialIndice& inVimMaterialIdToDsMeshMaterialIndice,
                                         CVimToDatasmith& inVimToDatasmith);
    
    // Return the first element
    const CMeshElement* GetFirstElement() const { return mFirstElement; }
    
private:
    // We keep the first created mesh element to reuse it's values (name, dimensions) for next ones
    CMeshElement* mFirstElement = nullptr;
    std::unordered_map<CMD5Hash, std::unique_ptr<CMeshElement>, CMD5Hash::SHasher> mMapMaterialMD5ToMeshElement;
};


// MeshElement
class CVimToDatasmith::CMeshElement {
public:
    // Constructor
    CMeshElement(const CMeshDefinition& inMeshDefinition, const MapVimMaterialIdToDsMeshMaterialIndice& inVimMaterialIdToDsMeshMaterialIndice,
                 const CMD5Hash& inMaterialsMD5Hash);
    
    // Called in the thread building our mesh assets
    void InitAsFirstElement(const TSharedPtr<IDatasmithMeshElement>& inFirstElement);
    
    // Copy first element mesh definition to this one.
    void InitWithFirstElement(const CMeshElement& inFirstElement);
    
    // Set mesh element with the materials
    void InitMeshMaterials(const CVimToDatasmith& inVimToDatasmith);
    
    // Return the mesh element (may create it)
    const IDatasmithMeshElement* GetMeshElement(const CVimToDatasmith& inVimToDatasmith) {
        if (mMeshElement.IsValid())
            return mMeshElement.Get();
        const CMeshElement* firstElement = mMeshDefinition.GetFirstElement();
        if (firstElement != nullptr && firstElement->mMeshElement.IsValid()) {
            InitWithFirstElement(*firstElement);
            if (mMeshElement.IsValid()) {
                InitMeshMaterials(inVimToDatasmith);
                std::unique_lock<std::mutex> lk(inVimToDatasmith.mConverter.GetSceneAccess());
                inVimToDatasmith.mConverter.GetScene()->AddMesh(mMeshElement);
                return mMeshElement.Get();
            }
        }
        return nullptr;
    }
    
private:
    const CMeshDefinition& mMeshDefinition; // The mesh definition (asset)
    MapVimMaterialIdToDsMeshMaterialIndice mVimMaterialIdToDsMeshMaterialIndice; // Vim material collected when creating mesh
    CMD5Hash mMaterialsMD5Hash; // Hash materials from the list over.
    
    TSharedPtr<IDatasmithMeshElement> mMeshElement; // The created mesh element (== mesh definition + materials used)
};


// GeometryEntry is a definition and an array of instances
class CVimToDatasmith::CGeometryEntry : CTaskMgr::ITask {
public:
    // Constructor
    CGeometryEntry(CVimToDatasmith* inVimToDatasmith, GeometryIndex inGeometry, NodeIndex inDefinition);
    
    // Add an instance.
    void AddInstance(NodeIndex inInstance);
    
    // Create Datasmith actors
    void CreateActors();
    
private:
    // Process the node's geometry (create datasmith mesh)
    void Run();
    
    void ConvertGeometryToDatasmithMesh(FDatasmithMesh* outMesh, MapVimMaterialIdToDsMeshMaterialIndice* outVimMaterialIdToDsMeshMaterialIndice);

    void AddActor(const TSharedRef<IDatasmithMeshActorElement>& inActor, NodeIndex inInstance);
    
    // Create an actor for the specified node
    void CreateActor(NodeIndex inInstance);
    
    // Create an efficient actor for the specified instance
    void CreateHierarchicalInstancesActor();
    
    // Create the actor name based on it's content (take care of duplicates)
    FString HashToName(Datasmith::FDatasmithHash& hasher, NodeIndex inInstance) const;
    
    CVimToDatasmith* const mVimToDatasmith; // The converter
    CMeshElement* mMeshElement = nullptr; // The mesh element that is geometry and affected material.
    GeometryIndex mGeometry = GeometryIndex::kNoGeometry;
    NodeIndex mDefinition = NodeIndex::kNoNode; // First instance is considered as the definition
    std::unique_ptr<std::vector<NodeIndex>> mInstances; // All other instances (exclude definition one)
};


// Constructor
CVimToDatasmith::CMaterialEntry::CMaterialEntry(MaterialId inVimId)
: mCount(0)
, mVimId(inVimId)
, mMaterialElement(FDatasmithSceneFactory::CreateUEPbrMaterial(UTF8_TO_TCHAR(Utf8StringFormat("%u", inVimId).c_str()))) {
}

// Copy constructor required to be in a std::vector
CVimToDatasmith::CMaterialEntry::CMaterialEntry(const CMaterialEntry& inOther)
: mCount((int32_t)inOther.mCount)
, mVimId(inOther.mVimId)
, mMaterialElement(inOther.mMaterialElement)
, mColor(inOther.mColor)
, mParams(inOther.mParams)
, mTexture(inOther.mTexture) {
}

IDatasmithMetaDataElement& CVimToDatasmith::CActorEntry::GetOrCreateMetadataElement(CVimToDatasmith* inVimToDatasmith) {
    if (!mMetaDataElement.IsValid()) {
        TestPtr(mActorElement);
        FString metadataName(FString::Printf(TEXT("MetaData_%s"), mActorElement->GetName()));
        mMetaDataElement = FDatasmithSceneFactory::CreateMetaData(*metadataName);
        mMetaDataElement->SetAssociatedElement(mActorElement);

        std::unique_lock<std::mutex> lk(inVimToDatasmith->mConverter.GetSceneAccess());
        inVimToDatasmith->mConverter.GetScene()->AddMetaData(mMetaDataElement);
    }
    return *mMetaDataElement;
}

// Constructor
CVimToDatasmith::CVimToDatasmith(CConvertVimToDatasmith* inConvertVimToDatasmith)
: mVim(inConvertVimToDatasmith->GetVim())
, mConverter(*inConvertVimToDatasmith) {
    mGeometryToDatasmithMeshMap[GeometryIndex::kNoGeometry] = TSharedPtr<IDatasmithMeshElement>();
}

// Destructor
CVimToDatasmith::~CVimToDatasmith() {
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
    if (!CreateFolder(TCHAR_TO_UTF8(*mVimToDatasmith->mConverter.GetOutputPath())))
        return;
    FString textureFolderPath = mVimToDatasmith->mConverter.GetOutputPath() + TEXT("/Textures");
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
        FString filePathName = mVimToDatasmith->mConverter.GetOutputPath() + TEXT("/Textures/") + mDatasmithName;
        filePathName += UTF8_TO_TCHAR(extension);
        mDatasmithTexture->SetFile(*filePathName);
        FMD5Hash FileHash = FMD5Hash::HashFile(mDatasmithTexture->GetFile());
        mDatasmithTexture->SetFileHash(FileHash);
#else
        mDatasmithTexture->SetData(mImageBuffer.data.begin(), uint32(mImageBuffer.data.size()), fmt);
#endif

        mDatasmithTexture->SetLabel(*mDatasmithLabel);
        mDatasmithTexture->SetSRGB(EDatasmithColorSpace::sRGB);
        std::unique_lock<std::mutex> lk(mVimToDatasmith->mConverter.GetSceneAccess());
        mVimToDatasmith->mConverter.GetScene()->AddTexture(mDatasmithTexture);
    }
}

CVimToDatasmith::CTextureEntry* CVimToDatasmith::CreateTexture(const utf8_t* inTextureName) {
    static const utf8_t texturePrefix[] = "textures\\";
    size_t l = strlen(texturePrefix);

    auto insertResult = mVimTextureToTextureMap.insert({inTextureName, std::unique_ptr<CTextureEntry>()});
    if (insertResult.second) {
        for (const auto& asset : mVim.GetAssetsBuffer()) {
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
    const Vim::EntityTable& materialTable = mVim.GetEntitiesTable("table:Rvt.Material");

    const std::vector<StringIndex>& nameArray = GetStringsColumn(materialTable, "Name");
    const std::vector<StringIndex>& textureNameArray = GetStringsColumn(materialTable, "ColorTextureFile");
    const std::vector<double>& idArray = GetNumericsColumn(materialTable, "Id");
    const std::vector<double>& colorXArray = GetNumericsColumn(materialTable, "Color.X");
    const std::vector<double>& colorYArray = GetNumericsColumn(materialTable, "Color.Y");
    const std::vector<double>& colorZArray = GetNumericsColumn(materialTable, "Color.Z");
    const std::vector<double>& transparencyArray = GetNumericsColumn(materialTable, "Transparency");
    const std::vector<double>& glossinessArray = GetNumericsColumn(materialTable, "Glossiness");
    const std::vector<double>& smoothnessArray = GetNumericsColumn(materialTable, "Smoothness");

    for (size_t i = 0; i < idArray.size(); i++) {
        MaterialId vimMaterialId = (MaterialId)idArray[i];

        VerboseF("CVimToDatasmith::CreateMaterials - MaterialId = %u\n", vimMaterialId);

        auto previous = mVimToDatasmithMaterialMap.find(vimMaterialId);
        if (previous != mVimToDatasmithMaterialMap.end()) {
            DebugF("MaterialId %u \"%s\" duplicated Index=%lu vs %lu\n", vimMaterialId, mVim.GetString(nameArray, i), i, previous->second);
            continue;
        }
        mVimToDatasmithMaterialMap[vimMaterialId] = mMaterials.size();

        CVimToDatasmith::CMaterialEntry materialEntry(vimMaterialId);

        materialEntry.mColor.x = colorXArray.size() > i ? (float)colorXArray[i] : 1.0f;
        materialEntry.mColor.y = colorYArray.size() > i ? (float)colorYArray[i] : 1.0f;
        materialEntry.mColor.z = colorZArray.size() > i ? (float)colorZArray[i] : 1.0f;
        materialEntry.mColor.w = transparencyArray.size() > i ? 1.0f - (float)transparencyArray[i] : 1.0f;

        materialEntry.mParams.x = glossinessArray.size() > i ? (float)glossinessArray[i] / 256.0f : 0.5f;
        materialEntry.mParams.y = smoothnessArray.size() > i ? (float)smoothnessArray[i] / 256.0f : 50.0f / 256.0f;

        const utf8_t* textureName = mVim.GetString(textureNameArray, i);
        if (*textureName)
            materialEntry.mTexture = CreateTexture(textureName);

        IDatasmithUEPbrMaterialElement& element = materialEntry.mMaterialElement.Get();
        const utf8_t* materialName = mVim.GetString(nameArray, i);
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

class CVimToDatasmith::CMetadataContext {
  public:
    CMetadataContext(CVimToDatasmith* inVimTodatasmith)
    : mVimTodatasmith(inVimTodatasmith)
    , mProperties(inVimTodatasmith->mVim.GetEntitiesTable("table:Rvt.Element").mProperties) {
        mStart = mProperties.size() > 0 ? &mProperties[0] : nullptr;
        mEnd = mStart + mProperties.size();
    }

    void Proceed() {
        const Vim::SerializableProperty* start;
        const Vim::SerializableProperty* end;
        CVimToDatasmith::CActorEntry* actorEntry = GetObject(&start, &end);
        while (actorEntry != nullptr) {
            while (start < end) {
                TSharedPtr<IDatasmithKeyValueProperty> dsProperty =
                    FDatasmithSceneFactory::CreateKeyValueProperty(UTF8_TO_TCHAR(mVimTodatasmith->mVim.GetString(StringIndex(start->mName))));
                dsProperty->SetValue(UTF8_TO_TCHAR(mVimTodatasmith->mVim.GetString(StringIndex(start->mValue))));
                dsProperty->SetPropertyType(EDatasmithKeyValuePropertyType::String);
                actorEntry->GetOrCreateMetadataElement(mVimTodatasmith).AddProperty(dsProperty);
                ++start;
            }
            actorEntry = GetObject(&start, &end);
        }
    }

  private:
    CVimToDatasmith::CActorEntry* GetObject(const Vim::SerializableProperty** outStart, const Vim::SerializableProperty** outEnd) {
        std::unique_lock<std::mutex> lk(mAccessControl);
        while (mStart < mEnd) {
            ElementIndex elementIndex = ElementIndex(mStart->mEntityId);
            *outStart = mStart;
            ++mStart;
            if (elementIndex < mVimTodatasmith->mVecElementToActors.size()) {
                CVimToDatasmith::CActorEntry& actorEntry = mVimTodatasmith->mVecElementToActors[elementIndex];
                if (actorEntry.HasElement()) {
                    while (mStart < mEnd && ElementIndex(mStart->mEntityId) == elementIndex)
                        ++mStart;
                    *outEnd = mStart;
                    return &actorEntry;
                }
            }
        }
        *outStart = nullptr;
        *outEnd = nullptr;
        return nullptr;
    }

    std::mutex mAccessControl;
    const Vim::SerializableProperty* mStart;
    const Vim::SerializableProperty* mEnd;
    CVimToDatasmith* const mVimTodatasmith;
    const std::vector<Vim::SerializableProperty>& mProperties;
};

void CVimToDatasmith::CreateAllMetaDatas() {
    mConverter.mBuildMetaDataTimeStat.BeginNow();
#if 1
    CMetadataContext metadataContext(this);
    CTaskMgr::CTaskJointer createAllMetaDatas("CreateAllMetaDatas");
    for (int i = 0; i < CTaskMgr::Get().GetNbProcessors(); ++i)
        (new CTaskMgr::TJoinableFunctorTask<CMetadataContext*>([](CMetadataContext* inMetadataContext) { inMetadataContext->Proceed(); }, &metadataContext))
            ->Start(&createAllMetaDatas);
    createAllMetaDatas.Join();
#else
    Vim::EntityTable& elementTable = mVimScene.mEntityTables["table:Rvt.Element"];
    for (auto& property : elementTable.mProperties) {
        ElementIndex elementIndex = ElementIndex(property.mEntityId);
        if (elementIndex != ElementIndex::kNoElement || elementIndex < mVecElementToActors.size()) {
            CActorEntry& actorEntry = mVecElementToActors[elementIndex];
            if (actorEntry.HasElement()) {
                TSharedPtr<IDatasmithKeyValueProperty> dsProperty =
                    FDatasmithSceneFactory::CreateKeyValueProperty(UTF8_TO_TCHAR(mVim.GetString(StringIndex(property.mName))));
                dsProperty->SetValue(UTF8_TO_TCHAR(mVim.GetString(StringIndex(property.mValue))));
                dsProperty->SetPropertyType(EDatasmithKeyValuePropertyType::String);
                actorEntry.GetOrCreateMetadataElement(this).AddProperty(dsProperty);
            }
        } else
            TraceF("CVimToDatasmith::CreateAllMetaDatas - Invalid element index %u\n", elementIndex);
    }
#endif
    mConverter.mBuildMetaDataTimeStat.FinishNow();
}

void CVimToDatasmith::CreateAllTags() {
    mConverter.mBuildTagsTimeStat.BeginNow();
    const Vim::EntityTable& elementTable = mVim.GetEntitiesTable("table:Rvt.Element");
    const std::vector<int>& elementToLevel = GetIndexColumn(elementTable, "Level:Level");
    const std::vector<int>& elementToCategory = GetIndexColumn(elementTable, "Category:Category");
    const std::vector<int>& elementToRoom = GetIndexColumn(elementTable, "Room:Room");
    const std::vector<StringIndex>& elementToFamilyName = GetStringsColumn(elementTable, "FamilyName");
    const std::vector<StringIndex>& elementToType = GetStringsColumn(elementTable, "Type");
    const std::vector<double>& elementToId = GetNumericsColumn(elementTable, "Id");

    for (ElementIndex elementIndex = ElementIndex(0); elementIndex < ElementIndex(mVecElementToActors.size()); Increment(elementIndex)) {
        IDatasmithActorElement* actor = mVecElementToActors[elementIndex].GetActorElement();
        if (actor != nullptr) {
            if (elementIndex < elementToId.size())
                actor->AddTag(*FString::Printf(TEXT("VIM.Id.%lld"), (long long)elementToId[elementIndex]));
            if (elementIndex < elementToLevel.size()) {
                int level = elementToLevel[elementIndex];
                if (level != -1)
                    actor->AddTag(*FString::Printf(TEXT("VIM.Level.%d"), level));
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
                    actor->AddTag(*FString::Printf(TEXT("VIM.Family.%s"), UTF8_TO_TCHAR(mVim.GetString(familyName))));
            }
            if (elementIndex < elementToType.size()) {
                StringIndex typeString = StringIndex(elementToType[elementIndex]);
                if (typeString != -1)
                    actor->AddTag(*FString::Printf(TEXT("VIM.Type.%s"), UTF8_TO_TCHAR(mVim.GetString(typeString))));
            }
        }
    }
    mConverter.mBuildTagsTimeStat.FinishNow();
}

// Create all definitions
void CVimToDatasmith::ProcessInstances() {
    VerboseF("CVimToDatasmith::ProcessDefinitions\n");
    mGeometryEntries.resize(mVim.mGroupIndexOffets.Count());
    mVecElementToActors.resize(mVim.mElementToName.Count());

    for (NodeIndex nodeIndex = NodeIndex(0); nodeIndex < mVim.mInstancesSubgeometry->Count(); nodeIndex = NodeIndex(nodeIndex + 1)) {
        GeometryIndex geometryIndex = (*mVim.mInstancesSubgeometry)[nodeIndex];

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
            std::unique_lock<std::mutex> lk(mConverter.GetSceneAccess());
            mConverter.GetScene()->AddMaterial(material.mMaterialElement);
        }
    }
}

void CVimToDatasmith::ConvertScene() {
    ConvertGeometries();
    CreateActors();
    AddUsedMaterials();
    CreateAllMetaDatas();
    CreateAllTags();
}

void CVimToDatasmith::ConvertGeometries() {
    try {
        ProcessInstances();
    } catch (...) {
        CTaskMgr::DeleteMgr(); // Kill all tasks
        throw;
    }
    CTaskMgr::Get().Join();
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
    const CMaterialEntry& materialEntry = mMaterials[materialIndex];
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
    FString OutputPath(FPaths::Combine(inVimToDatasmith.mConverter.GetOutputPath(), SubDir1, SubDir2));

    // Create a new mesh file
    FDatasmithMeshExporter MeshExporter;
    TSharedPtr<IDatasmithMeshElement> meshElement = MeshExporter.ExportToUObject(*OutputPath, inMesh.GetName(), inMesh, nullptr, EDSExportLightmapUV::Never);
    if (meshElement.IsValid()) {
        // meshElement->SetLabel(UTF8_TO_TCHAR(Utf8StringFormat("Geometry %d", geometryIndex).c_str()));
        for (auto& iter : inVimMaterialIdToDsMeshMaterialIndice) {
            size_t materialIndex = inVimToDatasmith.mVimToDatasmithMaterialMap[iter.first];
            TestAssert(materialIndex < inVimToDatasmith.mMaterials.size());
            CMaterialEntry& materialEntry = inVimToDatasmith.mMaterials[materialIndex];
            materialEntry.mCount++;
            meshElement->SetMaterial(materialEntry.mMaterialElement->GetName(), iter.second);
        }
        {
            std::unique_lock<std::mutex> lk(inVimToDatasmith.mConverter.GetSceneAccess());
            inVimToDatasmith.mConverter.GetScene()->AddMesh(meshElement);
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


// Convert geometry to Datasmith Mesh
void CVimToDatasmith::CGeometryEntry::ConvertGeometryToDatasmithMesh(FDatasmithMesh* outMesh,
                                                                     MapVimMaterialIdToDsMeshMaterialIndice* outVimMaterialIdToDsMeshMaterialIndice) {
    CVimImported& vim = mVimToDatasmith->mVim;
    outMesh->SetName(UTF8_TO_TCHAR(Utf8StringFormat("%d", mGeometry).c_str()));
    
    // Collect vertex used by this geometry
    std::unordered_map<VertexIndex, int32_t> vimIndiceToDsMeshIndice;
    int32_t verticesCount = 0;
    IndiceIndex indicesStart = vim.mGroupIndexOffets[mGeometry];
    IndiceIndex indicesEnd = IndiceIndex(indicesStart + vim.mGroupIndexCounts[mGeometry]);
    for (IndiceIndex index = indicesStart; index < indicesEnd; index = IndiceIndex(index + 1)) {
        VertexIndex vertexIndex = vim.mIndices[index];
        if (vimIndiceToDsMeshIndice.find(vertexIndex) == vimIndiceToDsMeshIndice.end())
            vimIndiceToDsMeshIndice[vertexIndex] = verticesCount++;
    }
    
    // Copy used vertex to the mesh
    outMesh->SetVerticesCount(verticesCount);
    for (const auto& iter : vimIndiceToDsMeshIndice) {
        const cVec3& position = vim.mPositions[iter.first];
        outMesh->SetVertex(iter.second, position.x * Meter2Centimeter, -position.y * Meter2Centimeter, position.z * Meter2Centimeter);
    }
    
    // Collect materials used by this geometry
    int32_t materialsCount = 0;
    FaceIndex vimMaterial = FaceIndex(indicesStart / 3);
    
    // Copy faces used by this geometry
    int32_t facesCount = vim.mGroupIndexCounts[mGeometry] / 3;
    TestAssert(facesCount * 3 == vim.mGroupIndexCounts[mGeometry]);
    outMesh->SetFacesCount(facesCount);
    IndiceIndex vimIndice = indicesStart;
#define ReportInvalid 0
#if ReportInvalid
    bool invalidReported = false;
    static uint32_t sReportedCount = 0;
#endif
    for (int32_t indexFace = 0; indexFace < facesCount; ++indexFace) {
        // Get material
        MaterialId vimMaterialId = vim.mMaterialIds[vimMaterial];
#if ReportInvalid
        if (vimMaterialId == kInvalidMaterial)
            if (invalidReported == false && sReportedCount < 10) {
                invalidReported = true;
                DebugF("CVimToDatasmith::CGeometryEntry::ConvertGeometryToDatasmithMesh - Geometry[%u], Invalid face material [%d]\n", mGeometry, indexFace);
                if (++sReportedCount == 10)
                    DebugF("CVimToDatasmith::CGeometryEntry::ConvertGeometryToDatasmithMesh - Report limit of 10 reached\n");
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
            VertexIndex indice = vim.mIndices[vimIndice];
            vimIndice = IndiceIndex(vimIndice + 1);
            triangleVertices[i] = vimIndiceToDsMeshIndice[indice];
            const cVec3& normal = vim.mNormals[indice];
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


void CVimToDatasmith::CGeometryEntry::Run() {
    if ((*mVimToDatasmith->mVim.mInstancesTransform)[mDefinition].IsIdentity()) {
    }

    FDatasmithMesh datasmithMesh;
    MapVimMaterialIdToDsMeshMaterialIndice vimMaterialIdToDsMeshMaterialIndice;
    ConvertGeometryToDatasmithMesh(&datasmithMesh, &vimMaterialIdToDsMeshMaterialIndice);

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

    ElementIndex elementIndex = mVimToDatasmith->mVim.mVimNodeToVimElement[inInstance];
    if (elementIndex != ElementIndex::kNoElement) {
        mVimToDatasmith->mVecElementToActors[elementIndex].SetActor(inActor, inInstance);
        inActor->SetLabel(UTF8_TO_TCHAR(mVimToDatasmith->mVim.GetString(mVimToDatasmith->mVim.mElementToName[elementIndex])));
    } else
        DebugF("CVimToDatasmith::CGeometryEntry::CreateActor - Invalid element (instance=%u)\n", inInstance);

    if (!*inActor->GetLabel())
        inActor->SetLabel(*FString::Printf(TEXT("Instance_%u"), inInstance));

    // Add the new actor to the scene
    std::unique_lock<std::mutex> lk(mVimToDatasmith->mConverter.GetSceneAccess());
    mVimToDatasmith->mConverter.GetScene()->AddActor(inActor);
}

void CVimToDatasmith::CGeometryEntry::CreateActor(NodeIndex inInstance) {
    FTransform actorTransfo(ToFTransform((*mVimToDatasmith->mVim.mInstancesTransform)[inInstance]));

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
    FTransform definitionTransfo(ToFTransform((*mVimToDatasmith->mVim.mInstancesTransform)[mDefinition]));
    hasher.HashQuat(definitionTransfo.GetRotation());
    hasher.HashFixVector(definitionTransfo.GetTranslation());
    hasher.HashScaleVector(definitionTransfo.GetScale3D());

    // Hash all instances
    for (NodeIndex instance : *mInstances) {
        FTransform instanceTransfo = ToFTransform((*mVimToDatasmith->mVim.mInstancesTransform)[instance]);
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
        FTransform instanceTransfo(ToFTransform((*mVimToDatasmith->mVim.mInstancesTransform)[instance]));
        hierarchicalMeshActor->AddInstance(instanceTransfo);
    }

    AddActor(hierarchicalMeshActor, mDefinition);
}

void CVimToDatasmith::CGeometryEntry::CreateActors() {
    const IDatasmithMeshElement* meshElement = mMeshElement != nullptr ? mMeshElement->GetMeshElement(*mVimToDatasmith) : nullptr;
    if (meshElement != nullptr) {
        if (mInstances == nullptr)
            CreateActor(mDefinition);
        else if (mVimToDatasmith->mConverter.GetNoHierarchicalInstance()) {
            CreateActor(mDefinition);
            for (NodeIndex instance : *mInstances)
                CreateActor(instance);
        } else
            CreateHierarchicalInstancesActor();
    }
}

} // namespace Vim2Ds
