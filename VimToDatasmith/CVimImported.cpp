// Copyright (c) 2021 VIM
// Licensed under the MIT License 1.0

#include "CVimImported.h"
#include "CTaskMgr.h"
#include "cMat.h"

namespace Vim2Ds {

static const std::vector<int> mEmptyIntVector;
static const std::vector<double> mEmptyDoubleVector;

const std::vector<int>& GetIndexColumn(const Vim::EntityTable& inTable, const std::string& inColumnName) {
    auto iterator = inTable.mIndexColumns.find(inColumnName);
    if (iterator == inTable.mIndexColumns.end())
        return mEmptyIntVector;
    return iterator->second;
}

const std::vector<StringIndex>& GetStringsColumn(const Vim::EntityTable& inTable, const std::string& inColumnName) {
    auto iterator = inTable.mStringColumns.find(inColumnName);
    if (iterator == inTable.mStringColumns.end())
        return reinterpret_cast<const std::vector<StringIndex>&>(mEmptyIntVector);
    return reinterpret_cast<const std::vector<StringIndex>&>(iterator->second);
}

const std::vector<double>& GetNumericsColumn(const Vim::EntityTable& inTable, const std::string& inColumnName) {
    auto iterator = inTable.mNumericColumns.find(inColumnName);
    if (iterator == inTable.mNumericColumns.end())
        return mEmptyDoubleVector;
    return iterator->second;
}

const Vim::EntityTable& CVimImported::GetEntitiesTable(const std::string& inEntityName) const {
    auto iterator = mVimScene.mEntityTables.find(inEntityName);
    TestAssert(iterator != mVimScene.mEntityTables.end());
    return iterator->second;
}

// Initialize the Vim scene with the vim file
void CVimImported::Read(const utf8_string& inVimFileName) {
    mReadTimeStat.BeginNow();
    Vim::VimErrorCodes vimReadResult = mVimScene.ReadFile(inVimFileName);
    if (vimReadResult != Vim::VimErrorCodes::Success)
        ThrowMessage("CVimImported::ReadVimFile - ReadFile return error %d", vimReadResult);
    mReadTimeStat.FinishNow();
#if 0
    DumpAssets();
#endif
#if 0
    DumpEntitiesTables();
#endif
}

void CVimImported::Prepare() {
    VerboseF("CVimImported::Prepare\n");

    CTaskMgr::CTaskJointer prepare("CVimImported::Prepare");
    CTaskMgr::CTaskJointer convertObsolete("ConvertObsolete");

    (new CTaskMgr::TJoinableFunctorTask<CVimImported*>([](CVimImported* inVimImporter) { inVimImporter->ConvertObsoleteSceneNode(); }, this))
        ->Start(&convertObsolete);

    Vim::EntityTable& nodeTable = mVimScene.mEntityTables["table:Vim.Node"];
    std::vector<int>& vimNodeToVimElement = nodeTable.mIndexColumns["Element:Element"];
    mVimNodeToVimElement.Initialize(vimNodeToVimElement);

    Vim::EntityTable& elementTable = mVimScene.mEntityTables["table:Rvt.Element"];
    std::vector<int>& elementToName = elementTable.mStringColumns["Name"];
    mElementToName.Initialize(elementToName);

#if 0
    DumpIndexColumn("table:Vim.Node", "Element:Element", vimNodeToVimElement);
    DumpStringColumn("table:Rvt.Element", "Name", elementToName);
#endif

    // Validate all node's element index
    (new CTaskMgr::TJoinableFunctorTask<CVimImported*>(
         [](CVimImported* inVimImporter) {
             VerboseF("CVimImported::CollectAttributes - X Begin\n");
             ElementIndex elementsCount = inVimImporter->mElementToName.Count();
             for (ElementIndex elementIndex : inVimImporter->mVimNodeToVimElement)
                 TestAssert(elementIndex == ElementIndex::kNoElement || elementIndex < elementsCount);
             VerboseF("CVimImported::CollectAttributes - X End\n");
         },
         this))
        ->Start(&prepare);

    convertObsolete.Join();

    (new CTaskMgr::TJoinableFunctorTask<CVimImported*>([](CVimImported* inVimImporter) { inVimImporter->CollectAttributes(); }, this))->Start(&prepare);

    prepare.Join();

    TestAssert(mVimNodeToVimElement.Count() == mInstancesSubgeometry->Count());
    PrintStats();
}

// In old vim files, the geometry is exported in world space, even when
// instanced, so we need to remove that world transform from the geometry
void CVimImported::FixOldVimFileTransforms() {
    if (mVimScene.mVersionMajor == 0 && mVimScene.mVersionMinor == 0 && mVimScene.mVersionPatch <= 200) {
        std::vector<bool> isTransformed(mGroupVertexOffets.Count(), false);

        for (NodeIndex nodeIndex = NodeIndex(0); nodeIndex < mInstancesSubgeometry->Count(); nodeIndex = NodeIndex(nodeIndex + 1)) {
            GeometryIndex subgeometry = (*mInstancesSubgeometry)[nodeIndex];
            if (subgeometry != kNoGeometry) {
                TestAssert(subgeometry < (int)isTransformed.size());
                if (!isTransformed[subgeometry]) {
                    isTransformed[subgeometry] = true;

                    cMat4 invTrans = (*mInstancesTransform)[nodeIndex].Inverse();

                    VertexIndex nextOffset = subgeometry < mGroupVertexOffets.Count() ? mGroupVertexOffets[GeometryIndex(subgeometry + 1)] : mPositions.Count();
                    for (VertexIndex index = mGroupVertexOffets[subgeometry]; index < nextOffset; index = VertexIndex(index + 1)) {
                        cVec3& vertex = mPositions[index];
                        vertex = vertex * invTrans;
                    }
                }
            }
        }
    }
}

void CVimImported::CollectAttributes() {
    TAttributeVector<cVec4> colorAttribute;
    VerboseF("CVimImported::CollectAttributes - Begin\n");

    constexpr const char* ObsoleteFaceGroupId = "g3d:face:groupid:0:int32:1";
    constexpr const char* ObsoleteGroupIndexOffset = "g3d:group:indexoffset:0:int32:1";
    constexpr const char* ObsoleteGroupVertexOffset = "g3d:group:vertexoffset:0:int32:1";

    for (const g3d::Attribute& attr : mVimScene.mGeometry.attributes) {
        auto descriptorString = attr.descriptor.to_string();
        if (descriptorString == g3d::descriptors::VertexColorWithAlpha)
            colorAttribute.Initialize(attr);
        else if (descriptorString == g3d::descriptors::Position)
            mPositions.Initialize(attr);
        else if (descriptorString == g3d::descriptors::Index)
            mIndices.Initialize(attr);
        else if (descriptorString == g3d::descriptors::FaceMaterialId)
            mMaterialIds.Initialize(attr);
        else if (descriptorString == g3d::descriptors::FaceGroup || descriptorString == ObsoleteFaceGroupId)
            mObjectIds.Initialize(attr);
        else if (descriptorString == g3d::descriptors::SubgeometryIndexOffset || descriptorString == ObsoleteGroupIndexOffset)
            mGroupIndexOffets.Initialize(attr);
        else if (descriptorString == g3d::descriptors::SubgeometryVertexOffset || descriptorString == ObsoleteGroupVertexOffset)
            mGroupVertexOffets.Initialize(attr);
        else if (descriptorString == g3d::descriptors::InstanceTransform) {
            TestAssert(!mInstancesTransform);
            mInstancesTransform.reset(new TAttributeVector<cMat4, NodeIndex>(attr));
        } else if (descriptorString == g3d::descriptors::InstanceParent) {
            TestAssert(!mInstancesParent);
            mInstancesParent.reset(new TAttributeVector<ParentIndex, NodeIndex>(attr));
        } else if (descriptorString == g3d::descriptors::InstanceSubgeometry) {
            TestAssert(!mInstancesSubgeometry);
            mInstancesSubgeometry.reset(new TAttributeVector<GeometryIndex, NodeIndex>(attr));
        } else if (descriptorString == g3d::descriptors::VertexUv)
            mVertexUVs.Initialize(attr);
        else
            TraceF("Unprocessed attribute \"%s\"\n", descriptorString.c_str());
    }

    TestAssert(mInstancesSubgeometry && mInstancesTransform && mInstancesParent);

    const size_t instancesCount = mInstancesSubgeometry->Count();
    TestAssert(mInstancesTransform->Count() == instancesCount && mInstancesParent->Count() == instancesCount);

    const GeometryIndex groupCount = mGroupIndexOffets.Count();
    TestAssert(groupCount == mGroupVertexOffets.Count());

    // Calculate group index counts
    mGroupIndexCounts.Allocate(groupCount);
    if (groupCount > 0) {
        for (GeometryIndex i = GeometryIndex(0); i < groupCount - 1; i = GeometryIndex(i + 1))
            mGroupIndexCounts[i] = IndiceIndex(mGroupIndexOffets[GeometryIndex(i + 1)] - mGroupIndexOffets[i]);
        mGroupIndexCounts[GeometryIndex(groupCount - 1)] = IndiceIndex(mIndices.Count() - mGroupIndexOffets[GeometryIndex(groupCount - 1)]);
    }

    FixOldVimFileTransforms();

    MeasureTime(ComputeNormals, ComputeNormals(), kP2DB_Verbose);

    VerboseF("CVimImported::CollectAttributes - End\n");
}

void CVimImported::ConvertObsoleteSceneNode() {
    VerboseF("CVimImported::ConvertObsoleteSceneNode - Begin\n");
    for (const auto& buffer : mVimScene.mBfast.buffers) {
        if (buffer.name == "nodes") {
            class CObsoleteSceneNode {
              public:
                ParentIndex mParent;
                GeometryIndex mGeometry;
                int mInstance; // Never used
                float mTransform[16];
            };
            NodeIndex instancesCount = NodeIndex(buffer.data.size() / sizeof(CObsoleteSceneNode));
            TestAssert(instancesCount * sizeof(CObsoleteSceneNode) == buffer.data.size());
            mInstancesTransform.reset(new TAllocatedVector<cMat4, NodeIndex>(instancesCount));
            mInstancesParent.reset(new TAllocatedVector<ParentIndex, NodeIndex>(instancesCount));
            mInstancesSubgeometry.reset(new TAllocatedVector<GeometryIndex, NodeIndex>(instancesCount));

            const CObsoleteSceneNode* nodes = reinterpret_cast<const CObsoleteSceneNode*>(buffer.data.begin());
            for (NodeIndex i = NodeIndex(0); i < instancesCount; i = NodeIndex(i + 1)) {
                (*mInstancesTransform)[i] = *reinterpret_cast<const cMat4*>(nodes->mTransform);
                (*mInstancesParent)[i] = nodes->mParent;
                (*mInstancesSubgeometry)[i] = nodes->mGeometry;
                ++nodes;
            }

            return;
        }
    }
    VerboseF("CVimImported::ConvertObsoleteSceneNode - End\n");
}

// Datasmith need normals.
void CVimImported::ComputeNormals() {
    mNormals.Allocate(mPositions.Count());
    for (IndiceIndex i = IndiceIndex(0); i < mIndices.Count(); i = IndiceIndex(i + 3)) {
        VertexIndex vi0 = mIndices[IndiceIndex(i + 0)];
        VertexIndex vi1 = mIndices[IndiceIndex(i + 1)];
        VertexIndex vi2 = mIndices[IndiceIndex(i + 2)];
        cVec3 v0 = mPositions[vi0];
        cVec3 v1 = mPositions[vi1];
        cVec3 v2 = mPositions[vi2];

        cVec3 s0 = v2 - v0;
        cVec3 s1 = v2 - v1;

        cVec3 normal = s1 ^ s0;
        normal.Normalise();

        mNormals[vi0] -= normal;
        mNormals[vi1] -= normal;
        mNormals[vi2] -= normal;
    }
    for (VertexIndex i = VertexIndex(0); i < mNormals.Count(); i = VertexIndex(i + 1))
        mNormals[i].Normalise();
}

void CVimImported::DumpStringColumn(const utf8_t* inTableName, const utf8_t* inColumnName, const std::vector<int>& inColumn) const {
    for (size_t i = 0; i < inColumn.size(); ++i)
        TraceF("\t[\"%s\"][\"%s\"][%lu] \"%s\"\n", inTableName, inColumnName, i, GetString(StringIndex(inColumn[i])));
}

void CVimImported::DumpTable(const utf8_t* inMsg, const Vim::EntityTable& inTable, bool inContent) const {
    for (auto iter : inTable.mIndexColumns) {
        TraceF("[\"%s\"] - mIndexColumns[%s] size=%lu\n", inMsg, iter.first.c_str(), iter.second.size());
        if (inContent)
            DumpIndexColumn(inMsg, iter.first.c_str(), iter.second);
    }
    for (auto iter : inTable.mStringColumns) {
        TraceF("[\"%s\"] - mStringColumns[%s] size=%lu\n", inMsg, iter.first.c_str(), iter.second.size());
        if (inContent)
            DumpStringColumn(inMsg, iter.first.c_str(), iter.second);
    }
    for (auto iter : inTable.mNumericColumns) {
        TraceF("[\"%s\"] - mNumericColumns[%s] size=%lu\n", inMsg, iter.first.c_str(), iter.second.size());
        if (inContent)
            DumpNumericColumn(inMsg, iter.first.c_str(), iter.second);
    }
    int previousEntityId = -1;
    for (auto& property : inTable.mProperties) {
        if (previousEntityId != property.mEntityId)
            TraceF("[\"%s\"] - Property id=%d\n", inMsg, property.mEntityId);
        TraceF("\t\t{ Name=\"%s\" Value=\"%s\" }\n", GetString(StringIndex(property.mName)), GetString(StringIndex(property.mValue)));
        previousEntityId = property.mEntityId;
    }
    TraceF("\n");
}

void CVimImported::DumpAssets() const {
    TraceF("Assets count= %ld\n", mVimScene.mAssetsBFast.buffers.size());
    for (const auto& asset : mVimScene.mAssetsBFast.buffers)
        TraceF("\t%s\n", asset.name.c_str());
}

void CVimImported::DumpEntitiesTables() const {
    for (auto table : mVimScene.mEntityTables)
        DumpTable(table.first.c_str(), table.second, true);
}

// Print selected contents
void CVimImported::PrintStats() {
    VerboseF("Positions %u, Indices=%u, Groups=%u, Instances=%u, Strings=%lu\n", mPositions.Count(), mIndices.Count(), mGroupVertexOffets.Count(),
             mInstancesSubgeometry->Count(), mVimScene.mStrings.size());

#if 0
#if 0
    TraceF("Instances count %u\n", mInstancesSubgeometry->Count());
    size_t validParentsCount = 0;
    size_t validGeometriesCount = 0;
    size_t validTransformCount = 0;
    for (NodeIndex index = NodeIndex(0); index < mInstancesSubgeometry->Count(); index = NodeIndex(index + 1)) {
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

} // namespace Vim2Ds
