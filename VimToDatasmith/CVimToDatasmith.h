// Copyright (c) 2021 VIM
// Licensed under the MIT License 1.0

#pragma once

#include "CConvertVimToDatasmith.h"
#include "CMD5Hash.h"
#include "CTaskMgr.h"
#include "CVimImported.h"

#include "cAABB.h"
#include "cQuat.h"

DISABLE_SDK_WARNINGS_START

#include "DatasmithMaterialElements.h"
#include "DatasmithSceneFactory.h"
#include "IDatasmithSceneElements.h"
#include "vim.h"

DISABLE_SDK_WARNINGS_END

#include <list>
#include <mutex>
#include <stack>

class FDatasmithMesh;

namespace Datasmith {
class FDatasmithHash;
}

namespace Vim2Ds {

class CConvertVimToDatasmith;

// Converter class
class CVimToDatasmith {
    class CTextureEntry;
    class CMaterialEntry;
    class CActorEntry;
    class CMeshElement;
    class CMeshDefinition;
    class CGeometryEntry;

    class CMetadatasProcessor;

  public:
    typedef std::unordered_map<MaterialId, int32_t> MapVimMaterialIdToDsMeshMaterialIndice;

    CVimImported& mVim;
    CConvertVimToDatasmith& mConverter;

    // Constructor
    CVimToDatasmith(CConvertVimToDatasmith* inConvertVimToDatasmith);

    // Destructor
    ~CVimToDatasmith();

    void ConvertScene();

    void ConvertGeometries();

    // Return the material name
    const TCHAR* GetMaterialName(MaterialId inVimMaterialId) const;

    // Compute the hash of the materials used
    CMD5Hash ComputeHash(const MapVimMaterialIdToDsMeshMaterialIndice& inVimMaterialIdToDsMeshMaterialIndice) const;

    // Create datasmith materials from Vim ones
    void CreateMaterials();

  private:
    // Get or create a texture entry
    CTextureEntry* CreateTexture(const utf8_t* inTextureName);

    // Create all definitions
    void ProcessInstances();

    // Create all actors
    void CreateActors();

    // Add Datasmith materials used to the scene
    void AddUsedMaterials();

    void CreateAllMetaDatas();

    void CreateAllTags();

    // All collected materials
    std::vector<CMaterialEntry> mMaterials;

    // Index permitting Vim material to our material entry index
    std::unordered_map<MaterialId, size_t> mVimToDatasmithMaterialMap;

    std::unordered_map<utf8_string, std::unique_ptr<CTextureEntry>> mVimTextureToTextureMap;

    std::vector<CActorEntry> mVecElementToActors;

    // Map Vim geometry index to datasmith mesh
    typedef std::unordered_map<GeometryIndex, TSharedPtr<IDatasmithMeshElement>> GeometryToDatasmithMeshMap;
    GeometryToDatasmithMeshMap mGeometryToDatasmithMeshMap;

    // List of already created mesh assets (the key is the MD5Hash of the mesh definition)
    std::unordered_map<CMD5Hash, std::unique_ptr<CMeshDefinition>, CMD5Hash::SHasher> mMeshDefinitions;
    std::mutex mDefinitionsAccessControl;

    std::vector<std::unique_ptr<CGeometryEntry>> mGeometryEntries; // vector of geometries

    std::map<std::string, const CGeometryEntry*> mMapInstancesNameToGeometry; // To detect name duplicate

    std::mutex mMultiPurposeAccessControl;
};

} // namespace Vim2Ds
