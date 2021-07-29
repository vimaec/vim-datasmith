// Copyright (c) 2021 VIM
// Licensed under the MIT License 1.0

#pragma once

#include "DebugTools.h"
#include "TVector.h"
#include "TimeStat.h"
#include "VimToDatasmith.h"

DISABLE_SDK_WARNINGS_START

#include "vim.h"

DISABLE_SDK_WARNINGS_END

namespace Vim2Ds {

class CVimImported {
  public:
    CVimImported() {}

    void Read(const utf8_string& inVimFileName);
    void Prepare();

    const utf8_t* GetString(StringIndex inIndex) const {
        TestAssert(inIndex < mVimScene.mStrings.size());
        return reinterpret_cast<const utf8_t*>(mVimScene.mStrings[inIndex]);
    }

    const utf8_t* GetString(const std::vector<StringIndex>& inStringIndex, size_t inIndex) const {
        if (inIndex >= inStringIndex.size())
            return "";
        return GetString(inStringIndex[inIndex]);
    }

    const bfast::vector<bfast::Buffer>& GetAssetsBuffer() const { return mVimScene.mAssetsBFast.buffers; }

    const Vim::EntityTable& GetEntitiesTable(const std::string& inEntityName) const;

    // Readed scene data
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

    // Working/Computed data
    TAllocatedVector<IndiceIndex, GeometryIndex> mGroupIndexCounts;
    TAllocatedVector<cVec3, VertexIndex> mNormals;

    FTimeStat mReadTimeStat;

    // For data exploration
    void DumpStringColumn(const utf8_t* inTableName, const utf8_t* inColumnName, const std::vector<int>& inColumn) const;
    void DumpTable(const utf8_t* inMsg, const Vim::EntityTable& inTable, bool inContent) const;
    void DumpAssets() const;
    void DumpEntitiesTables() const;

  private:
    void CollectAttributes();

    // In old vim files, the geometry is exported in world space, even when
    // instanced, so we need to remove that world transform from the geometry
    void FixOldVimFileTransforms();

    void ConvertObsoleteSceneNode();

    // Datasmith need normals.
    void ComputeNormals();

    Vim::Scene mVimScene;

    // Unprocessed vim data
    TAttributeVector<uint32_t> mObjectIds;
    TAttributeVector<float> mVertexUVs;
};

const std::vector<int>& GetIndexColumn(const Vim::EntityTable& inTable, const std::string& inColumnName);
const std::vector<StringIndex>& GetStringsColumn(const Vim::EntityTable& inTable, const std::string& inColumnName);
const std::vector<double>& GetNumericsColumn(const Vim::EntityTable& inTable, const std::string& inColumnName);

inline void DumpIndexColumn(const utf8_t* inTableName, const utf8_t* inColumnName, const std::vector<int>& inColumn) {
    for (size_t i = 0; i < inColumn.size(); ++i)
        TraceF("\t[\"%s\"][\"%s\"][%lu] %d\n", inTableName, inColumnName, i, inColumn[i]);
}

inline void DumpNumericColumn(const utf8_t* inTableName, const utf8_t* inColumnName, const std::vector<double>& inColumn) {
    for (size_t i = 0; i < inColumn.size(); ++i)
        TraceF("\t[\"%s\"][\"%s\"][%lu] %lf\n", inTableName, inColumnName, i, inColumn[i]);
}

} // namespace Vim2Ds
