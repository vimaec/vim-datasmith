// Copyright (c) 2021 VIM
// Licensed under the MIT License 1.0

#pragma once

#include "CTaskMgr.h"
#include "DebugTools.h"
#include "TimeStat.h"
#include "VimToDatasmith.h"

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

inline const cMat4& ToMat4(const Vim::SceneNode& inSceneNode) {
    return *reinterpret_cast<const cMat4*>(inSceneNode.mTransform);
}

#define UseDatasmithHierarchicalInstance \
    0 // IDatasmithHierarchicalInstancedStaticMeshActorElement currently not
      // supported by Tm

enum ESpecialValues {
    kNoParent = -1,
    kNoGeometry = -1,
    kNoInstance = -1,
    kInvalidIndex = -1,
};

// Basic vector class
template <class C> class TVector {
  public:
    // Return the size
    size_t Count() const { return mCount; }

    // Access to element  by it's index
    const C& operator[](size_t inIndex) const {
        TestAssertDebugOnly(inIndex < mCount);
        return mData[inIndex];
    }

    // Access to element  by it's index
    C& operator[](size_t inIndex) {
        TestAssertDebugOnly(mData != nullptr && inIndex < mCount);
        return mData[inIndex];
    }

    // Print the content of this vector
    void Print(const utf8_t* inDataName, const utf8_t* inSeparator = "\n\t\t") {
        TraceF("\t%s Count=%lu", inDataName, mCount);
        for (size_t index = 0; index < mCount; ++index)
            TraceF("%s%lu %s", inSeparator, index, ToUtf8(mData[index]));
        TraceF("\n");
    }

  protected:
    size_t mCount = 0; // Return the number of elements
    C* mData = nullptr; // Array of the elements
};

// Vector class that refers to attribute content.
/* Doesn't copy the attribute's content */
template <class C> class TAttributeVector : public TVector<C> {
  public:
    void Initialize(const g3d::Attribute& inAttr) {
        size_t dataSize = inAttr.byte_size();
        this->mCount = dataSize / sizeof(C);
        TestAssert(dataSize == this->mCount * sizeof(C));
        this->mData = reinterpret_cast<C*>(inAttr._begin);
    }
};

// Vector that allocate it's content
template <class C> class TAllocatedVector : public TVector<C> {
  public:
    // Destructor
    ~TAllocatedVector() { Clear(); }

    // Unallocate
    void Clear() {
        this->mCount = 0;
        delete[] this->mData;
        this->mData = nullptr;
    }

    // Allocate the required number of elements
    void Allocate(size_t inCount) {
        TestAssert(this->mData == nullptr);
        this->mCount = inCount;
        this->mData = new C[this->mCount]{};
    }
};

class CMD5Hash {
  public:
    CMD5Hash() {
        m[0] = 0;
        m[1] = 0;
    }

    CMD5Hash(size_t v1, size_t v2) {
        m[0] = v1;
        m[1] = v2;
    }

    CMD5Hash(const FMD5Hash& v) { memcpy(&m, v.GetBytes(), sizeof(m)); }

    CMD5Hash(FMD5* inMD5) { inMD5->Final(reinterpret_cast<uint8*>(m)); }

    CMD5Hash CombineWith(const CMD5Hash& inOther) { return CMD5Hash(m[0] ^ inOther.m[0], m[1] ^ inOther.m[1]); }

    FString ToString() const { return ((const FGuid*)m)->ToString(); }

    bool operator==(const CMD5Hash& inOther) const { return m[0] == inOther.m[0] && m[1] == inOther.m[1]; }

    struct SHasher {
        std::size_t operator()(const CMD5Hash& v) const { return v.m[0] ^ v.m[1]; }
    };

  private:
    size_t m[2];
};

// Converter class
class CVimToDatasmith {
  public:
    typedef std::unordered_map<uint32_t, int32_t> MapVimMaterialIdToDsMeshMaterialIndice;

    // Constructor
    CVimToDatasmith();

    // Destructor
    ~CVimToDatasmith();

    // Parse parameters to get Vim file path and datasmith file path
    void GetParameters(int argc, const utf8_t* const* argv);

    // Initialize the Vim scene with the vim file
    void ReadVimFile();

    // Convert Vim scene to a datasmith scene
    void CreateDatasmithScene();

    // Write a Datasmith scene to the Datasmith file
    void CreateDatasmithFile();

    // Return the material name
    const TCHAR* GetMaterialName(uint32_t inVimMaterialId) const;

    // Compute the hash of the materials used
    CMD5Hash ComputeHash(const MapVimMaterialIdToDsMeshMaterialIndice& inVimMaterialIdToDsMeshMaterialIndice) const;

    const utf8_t* GetVimString(int inIndex) const {
        TestAssert((unsigned)inIndex < mVimScene.mStrings.size());
        return reinterpret_cast<const utf8_t*>(mVimScene.mStrings[inIndex]);
    }

    const utf8_t* GetVimString(const std::vector<int>& indexStringArray, size_t inIndex) const {
        const utf8_t* theString = "";
        if (indexStringArray.size() > inIndex)
            theString = GetVimString(indexStringArray[inIndex]);
        return theString;
    }

  private:
    // Create datasmith materials from Vim ones
    void CreateMaterials();

    // Initialize the converter from Vim scene
    void ProcessGeometry();

    void ProcessDefinitions();

    // Create all mesh actors from Vim scene nodes
    void ProcessInstances();

    // Create all actors
    void CreateActors();

    // Add Datasmith materials used to the scene
    void AddUsedMaterials();

    // In old vim files, the geometry is exported in world space, even when
    // instanced, so we need to remove that world transform from the geometry
    void FixOldVimFileTransforms();

    // Datasmith need normals.
    void ComputeNormals(bool inFlip = false);

    // Print selected contents
    void PrintStats();

    // Convert geometry to Datasmith Mesh
    void ConvertGeometryToDatasmithMesh(int32 geometryIndex, FDatasmithMesh* outMesh,
                                        MapVimMaterialIdToDsMeshMaterialIndice* outVimMaterialIdToDsMeshMaterialIndice);

    // Extracted from parameters
    bool mNoHierarchicalInstance = false;
    std::string mVimFilePath;
    std::string mDatasmithFolderPath;
    std::string mDatasmithFileName;

    // Material's collected informations
    class MaterialEntry {
      public:
        // Constructor
        MaterialEntry(int32_t inVimId);

        // Copy constructor required to be in a std::vector
        MaterialEntry(const MaterialEntry& inOther);

        std::atomic<int32_t> mCount; // Number of mesh using this materials
        int32_t mVimId; // Vim material id
        cVec4 mColor; // Vim color
        cVec2 mParams; // Vim glossiness and smoothness

        TSharedRef<IDatasmithUEPbrMaterialElement> mMaterialElement; // Datasmith version of the materials
    };

    // All collected materials
    std::vector<MaterialEntry> mMaterials;

    // Index permitting Vim material to our material entry index
    std::unordered_map<uint32_t, size_t> mVimToDatasmithMaterialMap;

    // Map Vim geometry index to datasmith mesh
    typedef std::unordered_map<int32_t, TSharedPtr<IDatasmithMeshElement>> GeometryToDatasmithMeshMap;
    GeometryToDatasmithMeshMap mGeometryToDatasmithMeshMap;

    // Vim scene data
    Vim::Scene mVimScene;
    TAttributeVector<cVec3> mPositions;
    TAttributeVector<uint32_t> mIndices;
    TAttributeVector<uint32_t> mGroupIndexOffets;
    TAttributeVector<uint32_t> mGroupVertexOffets;
    TAttributeVector<uint32_t> mMaterialIds;
    TAttributeVector<uint32_t> mObjectIds;
    TAttributeVector<float> mVertexUVs;

    // Working/Computed data
    TAllocatedVector<uint32_t> mGroupIndexCounts;
    TAllocatedVector<cVec3> mNormals;

    // Datasmith scene and assets output path
    TSharedPtr<IDatasmithScene> mDatasmithScene;
    FString mOutputPath;

    // Conversion classes

    class CMeshElement; // Forward declaration

    // MeshDefinition
    // name = md5 of mesh data
    // Definition is use [1..N] MeshElement
    class CMeshDefinition {
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

    std::mutex mDefinitionsAccessControl;

    // List of already created mesh assets (the key is the MD5Hash of the mesh definition)
    std::unordered_map<CMD5Hash, std::unique_ptr<CMeshDefinition>, CMD5Hash::SHasher> mMeshDefinitions;

    // MeshElement
    class CMeshElement {
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
                    inVimToDatasmith.mDatasmithScene->AddMesh(mMeshElement);
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
    class CGeometryEntry : CTaskMgr::ITask {
      public:
        // Constructor
        CGeometryEntry(CVimToDatasmith* inVimToDatasmith, const Vim::SceneNode& inDefinition);

        // Add an instance.
        void AddInstance(const Vim::SceneNode* inInstance);

        // Create Datasmith actors
        void CreateActors();

      private:
        // Process the node's geometry (create datasmith mesh)
        void Run();

        // Create an actor for the specified node
        void CreateActor(const Vim::SceneNode& inSceneNode);

        // Create an efficient actor for the specified instance
        void CreateHierarchicalInstancesActor();

        // Create the actor name based on it's content (take care of duplicates)
        FString HashToName(Datasmith::FDatasmithHash& hasher, const FString& inLabel) const;

        CVimToDatasmith* const mVimToDatasmith; // The converter
        CMeshElement* mMeshElement = nullptr; // The mesh element that is geometry and affected material.
        const Vim::SceneNode& mDefinition; // First instance is considered as the definition
        std::unique_ptr<std::vector<const Vim::SceneNode*>> mInstances; // All other instances (exclude definition one)
    };

    std::vector<std::unique_ptr<CGeometryEntry>> mGeometryEntries; // vector of geometries

    std::map<std::string, const CGeometryEntry*> mMapInstancesNameToGeometry; // To detect name duplicate

    void DumpTable(const utf8_t* inMsg, const Vim::EntityTable& inTable, bool inContent) const;

    // For statistics purpose
    FTimeStat mStartTimeStat;
    FTimeStat mSetupTimeStat;
    FTimeStat mReadTimeStat;
    FTimeStat mPrepareTimeStat;
    FTimeStat mConvertTimeStat;
    FTimeStat mValidationTimeStat;
    FTimeStat mWriteTimeStat;

    void ReportTimeStat();
};

} // namespace Vim2Ds
