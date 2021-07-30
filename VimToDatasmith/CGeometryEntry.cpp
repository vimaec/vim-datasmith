// Copyright (c) 2021 VIM
// Licensed under the MIT License 1.0

#include "CGeometryEntry.h"

#include "CActorEntry.h"
#include "CMaterialEntry.h"
#include "CMeshDefinition.h"

#include "DatasmithHashTools.h"

DISABLE_SDK_WARNINGS_START

#include "Math/Transform.h"

DISABLE_SDK_WARNINGS_END

#include <iostream>

namespace Vim2Ds {

// Convert vim matrix to Unreal Engine tranformation
FTransform ToFTransform(const cMat4& inMat) {
    cQuat quat;
    quat.FromMat4(inMat);
    quat.Normalise();

    FQuat rotation(-quat.mQuat.x, quat.mQuat.y, -quat.mQuat.z, -quat.mQuat.w);
    FVector translation(inMat.m30 * Meter2Centimeter, -inMat.m31 * Meter2Centimeter, inMat.m32 * Meter2Centimeter);
    FVector scale(inMat.mRow0.Length(), inMat.mRow1.Length(), inMat.mRow2.Length());

    return FTransform(rotation, translation, scale);
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

// Process the node's geometry (create datasmith mesh)
void CVimToDatasmith::CGeometryEntry::Run() {
    FDatasmithMesh datasmithMesh;
    MapVimMaterialIdToDsMeshMaterialIndice vimMaterialIdToDsMeshMaterialIndice;
    ConvertGeometryToDatasmithMesh(&datasmithMesh, &vimMaterialIdToDsMeshMaterialIndice);

    // If material list isn't empty -> We have at least 1 face
    if (!vimMaterialIdToDsMeshMaterialIndice.empty()) {
        // Create an mesh id based on mesh content
        Datasmith::FDatasmithHash meshHasher;
        meshHasher.ComputeDatasmithMeshHash(datasmithMesh);
        FMD5Hash meshHash = meshHasher.GetHashValue();
        CMD5Hash meshMD5Hash(meshHash);

        // List of already created mesh assets (the key is the MD5Hash of the mesh definition)
        bool isNewDefinition = false;
        CMeshDefinition* meshDefinition = nullptr;
        {
            // Find an already existing or try to add a new one
            std::unique_lock<std::mutex> lk(mVimToDatasmith->mDefinitionsAccessControl);
            auto insertResult = mVimToDatasmith->mMeshDefinitions.insert({meshMD5Hash, std::unique_ptr<CMeshDefinition>()});
            if (insertResult.second) {
                insertResult.first->second.reset(new CMeshDefinition());
                isNewDefinition = true;
            }
            meshDefinition = insertResult.first->second.get();
        }
        if (isNewDefinition) {
            // We are the first, so we initialize the definition
            datasmithMesh.SetName(*LexToString(meshHash));
            mMeshElement = meshDefinition->Initialize(datasmithMesh, vimMaterialIdToDsMeshMaterialIndice, *mVimToDatasmith);
        } else // We are a new element of this definition
            mMeshElement = meshDefinition->GetOrCreateMeshElement(vimMaterialIdToDsMeshMaterialIndice, *mVimToDatasmith);
    }
}

// We add instance -> IDatasmithHierarchicalInstancedStaticMeshActorElement
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

// Finalize actor initialization and add it to the scene
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
    /* 1st Datasmith name are id (must be unique) while label is what is view by the user.
       2nd Ideally (for Datasmith reimport mechanism), name(id) must be unique and permanent (same for import and reimport).
       Vim file doesn't seem to provide id with these qualities.
       So to have a quasi permanent id - I based them on the content (mesh hash, mesh element hash, actor transformation) */
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

// Creat all actor using this geometry
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
