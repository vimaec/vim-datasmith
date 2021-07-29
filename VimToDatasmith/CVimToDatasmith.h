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

enum ElementIndex : uint32_t { kNoElement = (uint32_t)-1 };

enum NodeIndex : uint32_t { kNoNode = (uint32_t)-1 };

enum GeometryIndex : uint32_t { kNoGeometry = (uint32_t)-1 };

enum ParentIndex : uint32_t { kNoParent = (uint32_t)-1 };

enum VertexIndex : uint32_t { kInvalidVertex = (uint32_t)-1 };

enum GroupIndex : uint32_t { kInvalidGroup = (uint32_t)-1 };

enum MaterialId : uint32_t { kInvalidMaterial = (uint32_t)-1 };

enum IndiceIndex : uint32_t { kInvalidIndice = (uint32_t)-1 };

enum FaceIndex : uint32_t { kInvalidFace = (uint32_t)-1 }; // FaceIndex is same as Indices index / 3

enum StringIndex : uint32_t { kInvalidString = (uint32_t)-1 };

template <class Enum> inline void Increment(Enum& e) {
    e = Enum(e + 1);
}

// Basic vector class
template <class C, class Indexor = std::size_t> class TVector {
  public:
    virtual ~TVector(){};

    // Return the size
    Indexor Count() const { return Indexor(mCount); }

    // Access to element  by it's index
    const C& operator[](Indexor inIndex) const {
        TestAssertDebugOnly(inIndex < mCount);
        return mData[inIndex];
    }

    // Access to element  by it's index
    C& operator[](Indexor inIndex) {
        TestAssertDebugOnly(mData != nullptr && inIndex < mCount);
        return mData[inIndex];
    }

    // Print the content of this vector
    void Print(const utf8_t* inDataName, const utf8_t* inSeparator = "\n\t\t") {
        TraceF("\t%s Count=%lu", inDataName, mCount);
        for (Indexor index = Indexor(0); index < mCount; Increment(index))
            TraceF("%s%lu %s", inSeparator, index, ToUtf8(mData[index]));
        TraceF("\n");
    }

    const C* begin() const { return mData; }

    const C* end() const { return mData + mCount; }

  protected:
    size_t mCount = 0; // Return the number of elements
    C* mData = nullptr; // Array of the elements
};

// Vector class that refers to attribute content.
/* Doesn't copy the attribute's content */
template <class C, class Indexor = std::size_t> class TAttributeVector : public TVector<C, Indexor> {
  public:
    TAttributeVector() {}
    TAttributeVector(const g3d::Attribute& inAttr) { Initialize(inAttr); }

    void Initialize(const g3d::Attribute& inAttr) {
        size_t dataSize = inAttr.byte_size();
        this->mCount = dataSize / sizeof(C);
        TestAssert(dataSize == this->mCount * sizeof(C));
        this->mData = reinterpret_cast<C*>(inAttr._begin);
    }
};

// Vector class that refers to vector of integer.
/* Doesn't copy the vector */
template <class C, class Indexor> class TIndexor : public TVector<C, Indexor> {
    static_assert(sizeof(C) == sizeof(int), "C hasn't same size as int");

  public:
    TIndexor() {}
    TIndexor(const std::vector<int>& inOriginal) { Initialize(inOriginal); }

    void Initialize(const std::vector<int>& inOriginal) {
        this->mCount = inOriginal.size();
        if (this->mCount > 0)
            this->mData = const_cast<C*>(reinterpret_cast<const C*>(&inOriginal[0]));
    }
};

// Vector that allocate it's content
template <class C, class Indexor = std::size_t> class TAllocatedVector : public TVector<C, Indexor> {
  public:
    TAllocatedVector() {}
    TAllocatedVector(Indexor inCount) { Allocate(inCount); }

    // Destructor
    ~TAllocatedVector() { Clear(); }

    // Unallocate
    void Clear() {
        this->mCount = 0;
        delete[] this->mData;
        this->mData = nullptr;
    }

    // Allocate the required number of elements
    void Allocate(Indexor inCount) {
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

class CNamePtr {
  public:
    CNamePtr(const TCHAR* inName = nullptr)
    : mName(inName) {}

    operator const TCHAR*() const { return mName; }

    bool operator==(const CNamePtr& inOther) const {
        if (mName == inOther.mName)
            return true;
        if (mName == nullptr || inOther.mName == nullptr)
            return false;
        return FCString::Strcmp(mName, inOther.mName);
    }

    struct SHasher {
        std::size_t operator()(const CNamePtr& inName) const {
            int32 l = FCString::Strlen(inName.mName);
            int32 half = (l + 1) >> 1;
            return std::size_t(FCrc::Strihash_DEPRECATED(l - half, inName.mName + half)) << 32 | FCrc::Strihash_DEPRECATED(half, inName.mName);
        }
    };

  private:
    const TCHAR* mName;
};

// Converter class
class CVimToDatasmith {
  public:
    typedef std::unordered_map<MaterialId, int32_t> MapVimMaterialIdToDsMeshMaterialIndice;

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
    const TCHAR* GetMaterialName(MaterialId inVimMaterialId) const;

    // Compute the hash of the materials used
    CMD5Hash ComputeHash(const MapVimMaterialIdToDsMeshMaterialIndice& inVimMaterialIdToDsMeshMaterialIndice) const;

    const utf8_t* GetVimString(StringIndex inIndex) const {
        TestAssert(inIndex < mVimScene.mStrings.size());
        return reinterpret_cast<const utf8_t*>(mVimScene.mStrings[inIndex]);
    }

    const utf8_t* GetVimString(const std::vector<int>& indexStringArray, size_t inIndex) const {
        const utf8_t* theString = "";
        if (indexStringArray.size() > inIndex)
            theString = GetVimString(StringIndex(indexStringArray[inIndex]));
        return theString;
    }

  private:
    class CTextureEntry;
    CTextureEntry* CreateTexture(const utf8_t* inTextureName);

    // Create datasmith materials from Vim ones
    void CreateMaterials();

    void CollectAttributes();

    // Initialize the converter from Vim scene
    void ProcessGeometry();

    void ConvertObsoleteSceneNode();

    // Create all definitions
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
    void ConvertGeometryToDatasmithMesh(GeometryIndex geometryIndex, FDatasmithMesh* outMesh,
                                        MapVimMaterialIdToDsMeshMaterialIndice* outVimMaterialIdToDsMeshMaterialIndice);

    void CreateAllMetaDatas();
    void CreateAllTags();

    // Extracted from parameters
    bool mNoHierarchicalInstance = false;
    std::string mVimFilePath;
    std::string mDatasmithFolderPath;
    std::string mDatasmithFileName;

    // Material's collected informations
    class MaterialEntry {
      public:
        // Constructor
        MaterialEntry(MaterialId inVimId);

        // Copy constructor required to be in a std::vector
        MaterialEntry(const MaterialEntry& inOther);

        std::atomic<int32_t> mCount; // Number of mesh using this materials
        MaterialId mVimId; // Vim material id
        cVec4 mColor; // Vim color
        cVec2 mParams; // Vim glossiness and smoothness
        CTextureEntry* mTexture = nullptr;

        TSharedRef<IDatasmithUEPbrMaterialElement> mMaterialElement; // Datasmith version of the materials
    };

    // All collected materials
    std::vector<MaterialEntry> mMaterials;

    // Index permitting Vim material to our material entry index
    std::unordered_map<MaterialId, size_t> mVimToDatasmithMaterialMap;

    // Material's collected informations
    class CTextureEntry {
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

    std::unordered_map<utf8_string, std::unique_ptr<CTextureEntry>> mVimTextureToTextureMap;

    // Material's collected informations
    class CActorEntry {
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
    std::vector<CActorEntry> mVecElementToActors;

    // Map Vim geometry index to datasmith mesh
    typedef std::unordered_map<GeometryIndex, TSharedPtr<IDatasmithMeshElement>> GeometryToDatasmithMeshMap;
    GeometryToDatasmithMeshMap mGeometryToDatasmithMeshMap;

    // Vim scene data
    Vim::Scene mVimScene;
    TAttributeVector<cVec3, VertexIndex> mPositions;
    TAttributeVector<VertexIndex, IndiceIndex> mIndices;
    TAttributeVector<IndiceIndex, GeometryIndex> mGroupIndexOffets;
    TAttributeVector<VertexIndex, GeometryIndex> mGroupVertexOffets;
    TAttributeVector<MaterialId, FaceIndex> mMaterialIds;
    std::unique_ptr<TVector<cMat4, NodeIndex>> mInstancesTransform;
    std::unique_ptr<TVector<ParentIndex, NodeIndex>> mInstancesParent;
    std::unique_ptr<TVector<GeometryIndex, NodeIndex>> mInstancesSubgeometry;

    TIndexor<ElementIndex, NodeIndex> mVimNodeToVimElement;
    TIndexor<StringIndex, ElementIndex> mElementToName;

    // Unprocessed vim data
    TAttributeVector<uint32_t> mObjectIds;
    TAttributeVector<float> mVertexUVs;

    // Working/Computed data
    TAllocatedVector<IndiceIndex, GeometryIndex> mGroupIndexCounts;
    TAllocatedVector<cVec3, VertexIndex> mNormals;

    // Datasmith scene and assets output path
    TSharedPtr<IDatasmithScene> mDatasmithScene;
    FString mOutputPath;

#if 0
    size_t mNodesCount = 0;
    size_t GetNodesCount() const { return mNodesCount; }
#else
#endif

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
        CGeometryEntry(CVimToDatasmith* inVimToDatasmith, GeometryIndex inGeometry, NodeIndex inDefinition);

        // Add an instance.
        void AddInstance(NodeIndex inInstance);

        // Create Datasmith actors
        void CreateActors();

      private:
        // Process the node's geometry (create datasmith mesh)
        void Run();

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

    std::vector<std::unique_ptr<CGeometryEntry>> mGeometryEntries; // vector of geometries

    std::map<std::string, const CGeometryEntry*> mMapInstancesNameToGeometry; // To detect name duplicate

    void DumpStringColumn(const utf8_t* inTableName, const utf8_t* inColumnName, const std::vector<int>& inColumn) const;
    void DumpTable(const utf8_t* inMsg, const Vim::EntityTable& inTable, bool inContent) const;
    void DumpAssets() const;
    void DumpEntitiesTables() const;

    // For statistics purpose
    FTimeStat mStartTimeStat;
    FTimeStat mSetupTimeStat;
    FTimeStat mReadTimeStat;
    FTimeStat mPrepareTimeStat;
    FTimeStat mConvertTimeStat;
    FTimeStat mCreateMetaDataStat;
    FTimeStat mCreateTagsStat;
    FTimeStat mValidationTimeStat;
    FTimeStat mWriteTimeStat;

    void ReportTimeStat();
};

} // namespace Vim2Ds
