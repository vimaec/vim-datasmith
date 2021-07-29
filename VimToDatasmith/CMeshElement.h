// Copyright (c) 2021 VIM
// Licensed under the MIT License 1.0

#include "CConvertVimToDatasmith.h"
#include "CVimToDatasmith.h"

DISABLE_SDK_WARNINGS_START

#include "DatasmithMesh.h"
#include "DatasmithMeshExporter.h"
#include "Paths.h"

DISABLE_SDK_WARNINGS_END

namespace Vim2Ds {

// MeshElement
class CVimToDatasmith::CMeshElement {
  public:
    // Constructor
    CMeshElement(const CMeshDefinition& inMeshDefinition, const MapVimMaterialIdToDsMeshMaterialIndice& inVimMaterialIdToDsMeshMaterialIndice,
                 const CMD5Hash& inMaterialsMD5Hash)
    : mMeshDefinition(inMeshDefinition)
    , mVimMaterialIdToDsMeshMaterialIndice(inVimMaterialIdToDsMeshMaterialIndice)
    , mMaterialsMD5Hash(inMaterialsMD5Hash) {}

    // Called in the thread building our mesh assets
    void InitAsFirstElement(const TSharedPtr<IDatasmithMeshElement>& inFirstElement, CVimToDatasmith& inVimToDatasmith) {
        std::lock_guard<std::mutex> lock(inVimToDatasmith.mMultiPurposeAccessControl);
        mMeshElement = inFirstElement;
    }

    // Copy first element mesh definition to this one.
    void InitWithFirstElement(const CMeshElement& inFirstElement) {
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

    // Set mesh element with the materials
    void InitMeshMaterials(const CVimToDatasmith& inVimToDatasmith) {
        for (auto& iter : mVimMaterialIdToDsMeshMaterialIndice) {
            mMeshElement->SetMaterial(inVimToDatasmith.GetMaterialName(iter.first), iter.second);
        }
    }

    inline const CMeshElement* GetFirstElement() const; // mMeshDefinition.GetFirstElement()

    // Return the mesh element (may create it)
    const IDatasmithMeshElement* GetMeshElement(const CVimToDatasmith& inVimToDatasmith) {
        if (mMeshElement.IsValid())
            return mMeshElement.Get();
        const CMeshElement* firstElement = GetFirstElement();
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

} // namespace Vim2Ds
