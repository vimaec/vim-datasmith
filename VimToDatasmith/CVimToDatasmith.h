// Copyright (c) 2021 VIM
// Licensed under the MIT License 1.0

#pragma once

#include "DebugTools.h"
#include "VimToDatasmith.h"

DISABLE_SDK_WARNINGS_START

#include "DatasmithMaterialElements.h"
#include "IDatasmithSceneElements.h"
#include "cAABB.h"
#include "cQuat.h"
#include "vim.h"

DISABLE_SDK_WARNINGS_END

namespace Vim2Ds {

#define UseDatasmithHierarchicalInstance \
    0 // IDatasmithHierarchicalInstancedStaticMeshActorElement currently not
      // supported by Tm

// Basic vector class
template <class C> class TVector {
  public:
    // Return the size
    size_t Count() const { return mCount; }

    // Access to element  by it's index
    const C &operator[](size_t inIndex) const {
        TestAssertDebugOnly(inIndex < mCount);
        return mData[inIndex];
    }

    // Access to element  by it's index
    C &operator[](size_t inIndex) {
        TestAssertDebugOnly(mData != nullptr && inIndex < mCount);
        return mData[inIndex];
    }

    // Print the content of this vector
    void Print(const utf8_t *inDataName, const utf8_t *inSeparator = "\n\t\t") {
        TraceF("\t%s Count=%lu", inDataName, mCount);
        for (size_t index = 0; index < mCount; ++index)
            TraceF("%s%lu %s", inSeparator, index, ToUtf8(mData[index]));
        TraceF("\n");
    }

  protected:
    size_t mCount = 0; // Return the number of elements
    C *mData = nullptr; // Array of the elements
};

// Vector class that refers to attribute content.
/* Doesn't copy the attribute's content */
template <class C> class TAttributeVector : public TVector<C> {
  public:
    void Initialize(const g3d::Attribute &inAttr) {
        size_t dataSize = inAttr.byte_size();
        this->mCount = dataSize / sizeof(C);
        TestAssert(dataSize == this->mCount * sizeof(C));
        this->mData = reinterpret_cast<C *>(inAttr._begin);
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

// Converter class
class CVimToDatasmith {
  public:
    // Constructor
    CVimToDatasmith();

    // Destructor
    ~CVimToDatasmith();

    // Parse parameters to get Vim file path and datasmith file path
    void GetParameters(int argc, const utf8_t *argv[]);

    // Initialize the Vim scene with the vim file
    void ReadVimFile();

    // Convert Vim scene to a datasmith scene
    void CreateDatasmithScene();

    // Write a Datasmith scene to the Datasmith file
    void CreateDatasmithFile();

  private:
    // Create datasmith materials from Vim ones
    void CreateMaterials();

    // Initialize the converter from Vim scene
    void ProcessGeometry();

    // Create all mesh actors from Vim scene nodes
    void ProcessInstances();

    // Add Datasmith materials used to the scene
    void AddUsedMaterials();

    // In old vim files, the geometry is exported in world space, even when
    // instanced, so we need to remove that world transform from the geometry
    void FixOldVimFileTransforms();

    // Datasmith need normals.
    void ComputeNormals(bool inFlip = false);

    // Print selected contents
    void PrintStats();

    // Return (and create if necessary) the mesh for the specified geometry
    TSharedPtr<IDatasmithMeshElement> GetDatasmithMesh(int geometryIndex);

    // Create the mesh for the specified geometry
    TSharedPtr<IDatasmithMeshElement> CreateDatasmithMesh(int32 geometryIndex);

    // Extracted from parameters
    std::string mVimFilePath;
    std::string mDatasmithFolderPath;
    std::string mDatasmithFileName;

    // Material's collected informations
    class MaterialEntry {
      public:
        // Constructor
        MaterialEntry(int32_t inVimId);

        // Copy constructor required to be in a std::vector
        MaterialEntry(const MaterialEntry &inOther);

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
};

} // namespace Vim2Ds
