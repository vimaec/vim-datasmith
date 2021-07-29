// Copyright (c) 2021 VIM
// Licensed under the MIT License 1.0

#include "CMeshElement.h"

DISABLE_SDK_WARNINGS_START

#include "DatasmithMesh.h"
#include "DatasmithMeshExporter.h"
#include "Paths.h"

DISABLE_SDK_WARNINGS_END

namespace Vim2Ds {

// MeshDefinition
// name = md5 of mesh data
// Definition is use [1..N] MeshElement
class CVimToDatasmith::CMeshDefinition {
  public:
    // Constructor
    CMeshDefinition() {}

    // Initialize the mesh
    CMeshElement* Initialize(FDatasmithMesh& inMesh, const MapVimMaterialIdToDsMeshMaterialIndice& inVimMaterialIdToDsMeshMaterialIndice,
                             CVimToDatasmith& inVimToDatasmith) {
        TCHAR SubDir1[2] = {inMesh.GetName()[0], 0};
        TCHAR SubDir2[2] = {inMesh.GetName()[1], 0};
        FString OutputPath(FPaths::Combine(inVimToDatasmith.mConverter.GetOutputPath(), SubDir1, SubDir2));

        // Create a new mesh file
        FDatasmithMeshExporter MeshExporter;
        TSharedPtr<IDatasmithMeshElement> meshElement =
            MeshExporter.ExportToUObject(*OutputPath, inMesh.GetName(), inMesh, nullptr, EDSExportLightmapUV::Never);
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
        mFirstElement->InitAsFirstElement(meshElement, inVimToDatasmith);
        return mFirstElement;
    }

    // Return a mesh element for the material list specified.
    CMeshElement* GetOrCreateMeshElement(const MapVimMaterialIdToDsMeshMaterialIndice& inVimMaterialIdToDsMeshMaterialIndice,
                                         CVimToDatasmith& inVimToDatasmith) {
        CMD5Hash MD5Hash(inVimToDatasmith.ComputeHash(inVimMaterialIdToDsMeshMaterialIndice));

        std::lock_guard<std::mutex> lock(inVimToDatasmith.mMultiPurposeAccessControl);
        auto insertResult = mMapMaterialMD5ToMeshElement.insert({MD5Hash, std::unique_ptr<CMeshElement>()});
        if (insertResult.second)
            insertResult.first->second.reset(new CMeshElement(*this, inVimMaterialIdToDsMeshMaterialIndice, MD5Hash));
        return insertResult.first->second.get();
    }

    // Return the first element
    const CMeshElement* GetFirstElement() const { return mFirstElement; }

  private:
    // We keep the first created mesh element to reuse it's values (name, dimensions) for next ones
    CMeshElement* mFirstElement = nullptr;
    std::unordered_map<CMD5Hash, std::unique_ptr<CMeshElement>, CMD5Hash::SHasher> mMapMaterialMD5ToMeshElement;
};

inline const CVimToDatasmith::CMeshElement* CVimToDatasmith::CMeshElement::GetFirstElement() const {
    return mMeshDefinition.GetFirstElement();
}

} // namespace Vim2Ds
