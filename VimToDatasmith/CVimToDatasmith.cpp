// Copyright (c) 2021 VIM
// Licensed under the MIT License 1.0

#include "CVimToDatasmith.h"
#include "CActorEntry.h"
#include "CGeometryEntry.h"
#include "CMaterialEntry.h"
#include "CMeshDefinition.h"
#include "CMetadatasProcessor.h"
#include "CTextureEntry.h"

namespace Vim2Ds {

// Constructor
CVimToDatasmith::CVimToDatasmith(CConvertVimToDatasmith* inConvertVimToDatasmith)
: mVim(inConvertVimToDatasmith->GetVim())
, mConverter(*inConvertVimToDatasmith) {
    mGeometryToDatasmithMeshMap[GeometryIndex::kNoGeometry] = TSharedPtr<IDatasmithMeshElement>();
}

// Destructor
CVimToDatasmith::~CVimToDatasmith() {
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

void CVimToDatasmith::CreateAllMetaDatas() {
    mConverter.mBuildMetaDataTimeStat.BeginNow();
#if 1
    CMetadatasProcessor(this).Process();
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

} // namespace Vim2Ds
