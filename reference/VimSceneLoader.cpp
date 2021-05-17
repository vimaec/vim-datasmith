#include "VimSceneLoader.h"
#include "Scene.h"
#include "libEngine/Utils/cLogger.h"
#include "libEngine/Math/cVec.h"
#include "vim.h"

namespace libEngine
{
    class VimMaterial
    {
    public:
        cVec4 mColor;
        cVec2 mParams;
    };

    bool BuildMaterialMap(Vim::Scene& vimScene, std::unordered_map<uint32_t, VimMaterial>& materialMap)
    {
        SYSLOG(LOG_INFO) << "Building material map... ";
        auto& materialTable = vimScene.mEntityTables["table:Rvt.Material"];
        auto& idArray = materialTable.mNumericColumns["Id"];
        auto& colorXArray = materialTable.mNumericColumns["Color.X"];
        auto& colorYArray = materialTable.mNumericColumns["Color.Y"];
        auto& colorZArray = materialTable.mNumericColumns["Color.Z"];
        auto& transparencyArray = materialTable.mNumericColumns["Transparency"];
        auto& glossinessArray = materialTable.mNumericColumns["Glossiness"];
        auto& smoothnessArray = materialTable.mNumericColumns["Smoothness"];

        for (size_t i = 0; i < idArray.size(); i++)
        {
            VimMaterial material;
            material.mColor.x = colorXArray.size() > i ? (float)colorXArray[i] : 1.0f;
            material.mColor.y = colorYArray.size() > i ? (float)colorYArray[i] : 1.0f;
            material.mColor.z = colorZArray.size() > i ? (float)colorZArray[i] : 1.0f;
            material.mColor.w = transparencyArray.size() > i ? 1.0f - (float)transparencyArray[i] : 1.0f;
            material.mParams.x = glossinessArray.size() > i ? (float)glossinessArray[i] / 256.0f : 0.5f;
            material.mParams.y = smoothnessArray.size() > i ? (float)smoothnessArray[i] / 256.0f : 50.0f / 256.0f;

            materialMap[(uint32_t)idArray[i]] = material;
        }

        SYSLOG(LOG_INFO) << endl;

        return true;
    }

    uint32_t nig = 0;
    bool ProcessGeometry(Vim::Scene& vimScene, std::unordered_map<uint32_t, VimMaterial>& materialMap, VimSceneData* vimSceneData)
    {
        SYSLOG(LOG_INFO) << "Processing geometry... " << indent;

        vimSceneData->mVertexBuffer.mIndices = nullptr;
        vimSceneData->mVertexBuffer.mVertexCount = 0;

        uint32_t numInstanceGroups = 0;
        uint32_t* groupVertexOffets = nullptr;
        uint32_t* materialIds = nullptr;
        uint32_t* mObjectIds = {};
        uint32_t mTotalNumMaterialIds = 0u;
        uint32_t mTotalNumObjectIds = 0u;
        cVec4* colorAttribute = nullptr;

        for (auto& attr : vimScene.mGeometry.attributes)
        {
            auto descriptorString = attr.descriptor.to_string();
            if (descriptorString == g3d::descriptors::VertexColorWithAlpha)
            {
                uint64_t dataSize = attr._end - attr._begin;
                colorAttribute = (cVec4*)attr._begin;
            }
            else if (descriptorString == g3d::descriptors::Position)
            {
                uint64_t dataSize = attr._end - attr._begin;
                vimSceneData->mVertexBuffer.mVertexCount = (uint32_t)(dataSize / sizeof(cVec3));

                vimSceneData->mVertexBuffer.mPositions = tracked_new cVec3[vimSceneData->mVertexBuffer.mVertexCount];
                memcpy(vimSceneData->mVertexBuffer.mPositions, attr._begin, dataSize);
            }
            else if (descriptorString == g3d::descriptors::Index)
            {
                uint64_t dataSize = attr._end - attr._begin;
                vimSceneData->mVertexBuffer.mIndexCount = (uint32_t)(dataSize / sizeof(int));
                vimSceneData->mVertexBuffer.mIndices = tracked_new uint32_t[vimSceneData->mVertexBuffer.mIndexCount];
                memcpy(vimSceneData->mVertexBuffer.mIndices, attr._begin, dataSize);
            }
            else if (descriptorString == g3d::descriptors::FaceMaterialId)
            {
                uint64_t dataSize = attr._end - attr._begin;
                mTotalNumMaterialIds = (uint32_t)(dataSize / sizeof(int));
                materialIds = tracked_new uint32_t[mTotalNumMaterialIds];
                memcpy(materialIds, attr._begin, dataSize);
            }
            else if (descriptorString == g3d::descriptors::FaceGroupId)
            {
                // Object Ids
                uint64_t dataSize = attr._end - attr._begin;
                mTotalNumObjectIds = (uint32_t)(dataSize / sizeof(int));
                mObjectIds = tracked_new uint32_t[mTotalNumObjectIds];
                memcpy(mObjectIds, attr._begin, dataSize);
            }
            else if (descriptorString == g3d::descriptors::GroupIndexOffset)
            {
                numInstanceGroups = (uint32_t)attr.byte_size() / sizeof(int);
                nig = numInstanceGroups;
                vimSceneData->mGroupIndexOffets = (uint32_t*)attr._begin;
            }
            else if (descriptorString == g3d::descriptors::GroupVertexOffset)
            {
                groupVertexOffets = (uint32_t*)attr._begin;
            }
        }

        vimSceneData->mVertexBuffer.mColors = tracked_new ubyte4[vimSceneData->mVertexBuffer.mVertexCount];
        vimSceneData->mVertexBuffer.mObjectIds = tracked_new uint32_t[vimSceneData->mVertexBuffer.mVertexCount];
        vimSceneData->mVertexBuffer.mMaterialParams = tracked_new ubyte4[vimSceneData->mVertexBuffer.mVertexCount];

        vimSceneData->mGroupIndexCounts = tracked_new uint32_t[numInstanceGroups];
        vimSceneData->mGroupVertexCounts = tracked_new uint32_t[numInstanceGroups];

        // Calculate group offsets and counts
        if (numInstanceGroups > 0)
        {
            for (uint32_t i = 0; i < numInstanceGroups - 1; i++)
            {
                vimSceneData->mGroupIndexCounts[i] = vimSceneData->mGroupIndexOffets[i + 1] - vimSceneData->mGroupIndexOffets[i];
                vimSceneData->mGroupVertexCounts[i] = groupVertexOffets[i + 1] - groupVertexOffets[i];
            }
            vimSceneData->mGroupIndexCounts[numInstanceGroups - 1] = vimSceneData->mVertexBuffer.mIndexCount - vimSceneData->mGroupIndexOffets[numInstanceGroups - 1];
            vimSceneData->mGroupVertexCounts[numInstanceGroups - 1] = vimSceneData->mVertexBuffer.mVertexCount - groupVertexOffets[numInstanceGroups - 1];
        }

        vimSceneData->mGeometryAABBMap = tracked_new cAABB[numInstanceGroups];
        uint32_t* geometryObjectIdMap = tracked_new uint32_t[numInstanceGroups]{};
        vimSceneData->mIsAlphaInstanceMap = tracked_new bool[numInstanceGroups] {};
        bool* hasBeenTransformed = tracked_new bool[numInstanceGroups] {};
        for (size_t nodeIndex = 0; nodeIndex < vimScene.mNodes.size(); nodeIndex++)
        {
            const Vim::SceneNode& node = vimScene.mNodes[nodeIndex];
            if (node.mGeometry != -1)
            {
                if (!hasBeenTransformed[node.mGeometry])
                {
                    hasBeenTransformed[node.mGeometry] = true;
                    assert(node.mInstance == nodeIndex || node.mInstance == -1);

                    cVec3 minPoint(FLT_MAX);
                    cVec3 maxPoint(-FLT_MAX);
                    bool isAlphaInstance = false;
                    uint32_t objectId = 0xffffffff;
                    // In old vim files, the geometry is exported in world space, even when 
                    // instanced, so we need to remove that world transform from the geometry
                    bool removeGeometryTransform = vimScene.mVersionMajor == 0 && vimScene.mVersionMinor == 0 && vimScene.mVersionPatch <= 200;
                    if (removeGeometryTransform)
                    {
                        cMat4 trans = (*(cMat4*)node.mTransform);
                        cMat4 invTrans = trans.Inverse();

                        for (uint32_t index = 0; index < vimSceneData->mGroupVertexCounts[node.mGeometry]; index++)
                        {
                            cVec3& vertex = vimSceneData->mVertexBuffer.mPositions[index + groupVertexOffets[node.mGeometry]];
                            vertex = vertex * invTrans;
                            vertex = vertex;
                        }
                    }

                    for (uint32_t index = vimSceneData->mGroupIndexOffets[node.mGeometry];
                        index < vimSceneData->mGroupIndexOffets[node.mGeometry] + vimSceneData->mGroupIndexCounts[node.mGeometry];
                        index += 3)
                    {
                        auto materialId = materialIds[index / 3];
                        auto faceObjectId = mObjectIds[index / 3];

                        if (objectId == 0xffffffff)
                        {
                            objectId = faceObjectId;
                        }
                        else
                        {
                            //        assert(objectId == faceObjectId);
                        }

                        cVec4 color(0.2f, 0.2f, 0.2f, 1.0f);
                        cVec2 params(0.5f, 1.0f);
                        bool useColorAttribute = colorAttribute != nullptr;
                        if (materialId != 0xffffffff && materialMap.find(materialId) != materialMap.end())
                        {
                            useColorAttribute = false;
                            color = materialMap[materialId].mColor;
                            params = materialMap[materialId].mParams;
                            if (color.x == 0.0f && color.y == 0.0f && color.z == 0.0f)
                            {
                                color.Set(0.2f, 0.2f, 0.2f, 1.0f);
                            }

                            if (color.w < 0.99f)
                            {
                                isAlphaInstance = true;
                            }
                        }

                        for (uint32_t j = 0; j < 3; j++)
                        {
                            uint32_t pointIndex = vimSceneData->mVertexBuffer.mIndices[index + j];
                            if (useColorAttribute)
                            {
                                color = colorAttribute[pointIndex];
                            }

                            vimSceneData->mVertexBuffer.mColors[pointIndex] = color;
                            vimSceneData->mVertexBuffer.mObjectIds[pointIndex] = faceObjectId;
                            vimSceneData->mVertexBuffer.mMaterialParams[pointIndex] = cVec4(params.x, params.y, 0.0f, 0.0f);
                            cVec3 point = vimSceneData->mVertexBuffer.mPositions[pointIndex];

                            minPoint = minPoint.Min(point);
                            maxPoint = maxPoint.Max(point);
                        }
                    }

                    cAABB geometryAABB(minPoint, maxPoint);
                    assert(node.mGeometry < (int)numInstanceGroups);
                    vimSceneData->mGeometryAABBMap[node.mGeometry] = geometryAABB;
                    geometryObjectIdMap[node.mGeometry] = objectId;
                    vimSceneData->mIsAlphaInstanceMap[node.mGeometry] = isAlphaInstance;
                }
            }
        }

        delete[] geometryObjectIdMap;
        delete[] hasBeenTransformed;
        delete[] materialIds;

        // calculate some normals
        vimSceneData->mVertexBuffer.GenerateNormals(true); // Transforms from vims are changing the normal orientation, so we have to flip them

        vimSceneData->mMeshRef = sSceneRenderer.RegisterRenderMesh(vimSceneData->mVertexBuffer);

        SYSLOG(LOG_INFO) << outdent << endl;

        return true;
    }

    bool ProcessInstances(Vim::Scene& vimScene, Scene* scene, VimSceneData* vimSceneData, cMat4 transform)
    {
        SYSLOG(LOG_INFO) << "Processing Instances... " << indent;
        
        vimSceneData->mMinHeight = FLT_MAX;
        vimSceneData->mMaxHeight = -FLT_MAX;

        // Calculate Gaussian distribution of geometry
        cVec3 aabbSum(0.0f, 0.0f, 0.0f);
        cVec3 aabbSumSqr(0.0f, 0.0f, 0.0f);
        float aabbCount = 0.0f;

        for (size_t nodeIndex = 0; nodeIndex < vimScene.mNodes.size(); nodeIndex++)
        {
            const Vim::SceneNode& node = vimScene.mNodes[nodeIndex];
            int geometryIndex = -1;
            if (node.mGeometry != -1 && (node.mInstance == -1 || node.mInstance == nodeIndex))
            {
                geometryIndex = node.mGeometry;
            }
            else if (node.mInstance != -1 && node.mInstance != nodeIndex && vimScene.mNodes[node.mInstance].mGeometry != -1)
            {
                geometryIndex = vimScene.mNodes[node.mInstance].mGeometry;
            }

            if (geometryIndex != -1)
            {
                cMat4 trans = (*(cMat4*)node.mTransform).Transposed();
                trans.mRow0 = -trans.mRow0;
                cVec4 t = trans.mRow1;
                trans.mRow1 = -trans.mRow2;
                trans.mRow2 = t;

                assert(geometryIndex < (int)nig);
                cAABB geometryAABB = vimSceneData->mGeometryAABBMap[geometryIndex];
                cSphere boundingSphere(trans * geometryAABB.GetCenter(), geometryAABB.GetSize().GetMaxComponent() * 0.5f); // TODO: Improve world bounding box?
                if (
                    boundingSphere.mRadius == 0.0f ||
                    isnan(boundingSphere.mRadius) ||
                    isinf(boundingSphere.mRadius) ||
                    boundingSphere.mPos.IsNan() ||
                    boundingSphere.mPos.IsInf()
                    )
                {
                    SYSLOG(LOG_WARN) << "Invalid instance AABB. NodeIndex: " << nodeIndex << endl;
                    continue;
                }

                cVec3 minPos = boundingSphere.mPos - boundingSphere.mRadius;
                cVec3 maxPos = boundingSphere.mPos + boundingSphere.mRadius;

                float weight = boundingSphere.mRadius;
                aabbSum += minPos * weight;
                aabbSum += maxPos * weight;
                aabbSumSqr += minPos * minPos * weight;
                aabbSumSqr += maxPos * maxPos * weight;

                aabbCount += 2.0f * weight;
            }
        }

        cVec3 aabbMean = aabbSum / aabbCount;
        cVec3 aabbVariance = (aabbSumSqr - aabbSum * aabbMean) / aabbCount;

        // Generate a clipping aabb from the mean and variance of the Gaussian distribution
        cVec3 aabbCenter = aabbMean;
        cVec3 aabbExtent = aabbVariance.Sqrt() * 3.0f;
        cAABB clippingAABB(aabbCenter - aabbExtent, aabbCenter + aabbExtent);
        cAABB mainAABB(cVec3(FLT_MAX), cVec3(-FLT_MAX));

        for (size_t nodeIndex = 0; nodeIndex < vimScene.mNodes.size(); nodeIndex++)
        {
            const Vim::SceneNode& node = vimScene.mNodes[nodeIndex];
            int geometryIndex = -1;
            if (node.mGeometry != -1 && (node.mInstance == -1 || node.mInstance == nodeIndex))
            {
                geometryIndex = node.mGeometry;
            }
            else if (node.mInstance != -1 && node.mInstance != nodeIndex && vimScene.mNodes[node.mInstance].mGeometry != -1)
            {
                geometryIndex = vimScene.mNodes[node.mInstance].mGeometry;
            }

            if (geometryIndex != -1)
            {
                cMat4 trans = (*(cMat4*)node.mTransform).Transposed();
                trans.mRow0 = -trans.mRow0;
                cVec4 t = trans.mRow1;
                trans.mRow1 = -trans.mRow2;
                trans.mRow2 = t;

                cMat4 finalTrans = trans.Transposed();

                assert(geometryIndex < (int)nig);
                cAABB geometryAABB = vimSceneData->mGeometryAABBMap[geometryIndex];
                cSphere boundingSphere(trans * geometryAABB.GetCenter(), geometryAABB.GetSize().GetMaxComponent() * 0.5f); // TODO: Improve world bounding box?
                if (
                    boundingSphere.mRadius == 0.0f ||
                    isnan(boundingSphere.mRadius) ||
                    isinf(boundingSphere.mRadius) ||
                    boundingSphere.mPos.IsNan() ||
                    boundingSphere.mPos.IsInf()
                    )
                {
                    SYSLOG(LOG_WARN) << "Invalid instance AABB. NodeIndex: " << nodeIndex << endl;
                    continue;
                }

                cVec3 minPos = boundingSphere.mPos - boundingSphere.mRadius;
                cVec3 maxPos = boundingSphere.mPos + boundingSphere.mRadius;

                if (
                    clippingAABB.mMax.x > minPos.x &&
                    clippingAABB.mMax.y > minPos.y &&
                    clippingAABB.mMax.z > minPos.z &&
                    clippingAABB.mMin.x < maxPos.x &&
                    clippingAABB.mMin.y < maxPos.y &&
                    clippingAABB.mMin.z < maxPos.z
                    )
                {
                    mainAABB.Add(minPos);
                    mainAABB.Add(maxPos);
                }

                uint32_t meshStartIndex = vimSceneData->mGroupIndexOffets[geometryIndex] + vimSceneData->mMeshRef.mStartIndex;
                uint32_t meshIndexCount = vimSceneData->mGroupIndexCounts[geometryIndex];

                int32_t elementIndex = -1;
                if (meshStartIndex < vimSceneData->mVertexBuffer.mIndexCount)
                {
                    elementIndex = vimSceneData->mVertexBuffer.mObjectIds[vimSceneData->mVertexBuffer.mIndices[meshStartIndex]];
                }

                if (vimSceneData->mIsAlphaInstanceMap[geometryIndex])
                {
                    vimSceneData->mInstances.push_back(
                        scene->AddAlphaInstance({
                                geometryAABB,
                                meshStartIndex,
                                meshIndexCount
                            },
                            finalTrans * transform,
                            ubyte4(255, 255, 255, 255),
                            libEngine::InstanceFlags::HeightCull,
                            (int32_t)nodeIndex,
                            elementIndex
                        ));
                }
                else
                {
                    vimSceneData->mInstances.push_back(
                        scene->AddInstance({
                                geometryAABB,
                                meshStartIndex,
                                meshIndexCount
                            },
                            finalTrans* transform,
                            ubyte4(255, 255, 255, 255),
                            libEngine::InstanceFlags::HeightCull,
                            (int32_t)nodeIndex,
                            elementIndex
                        ));
                }
            }
        }

        delete[] vimSceneData->mGroupIndexCounts;
        delete[] vimSceneData->mGroupVertexCounts;
        delete[] vimSceneData->mGeometryAABBMap;
        delete[] vimSceneData->mIsAlphaInstanceMap;
        vimSceneData->mGroupIndexCounts = nullptr;
        vimSceneData->mGroupVertexCounts = nullptr;
        vimSceneData->mGroupIndexOffets = nullptr;
        vimSceneData->mGeometryAABBMap = nullptr;
        vimSceneData->mIsAlphaInstanceMap = nullptr;

        vimSceneData->mAABBCenter = (mainAABB.mMin + mainAABB.mMax) * 0.5f;
        vimSceneData->mAABBExtent = (mainAABB.mMax - mainAABB.mMin);
        vimSceneData->mMaxHeight = -mainAABB.mMin.y;
        vimSceneData->mMinHeight = -mainAABB.mMax.y;

        scene->mAABB.Add(mainAABB);

//        SYSLOG(LOG_INFO) << endl;
   //     SYSLOG(LOG_INFO) << "Total Render instances in VIM scene: " << mInstanceBuffer.size() << endl;
   //     SYSLOG(LOG_INFO) << "Total Alpha Render instances in VIM scene: " << mAlphaInstanceBuffer.size() << endl;
        SYSLOG(LOG_INFO) << outdent << endl;

        return true;
    }

    ErrorCodes VimSceneLoader::LoadVim(std::string fileName, Scene* scene, VimSceneData** outVimSceneData, cMat4 transform)
    {
        SYSLOG(LOG_INFO) << "Importing Vim file: " << fileName << indent;

        if (fileName[0] == '\"')
        {
            fileName = fileName.substr(1);
        }

        if (fileName[fileName.length() - 1] == '\"')
        {
            fileName = fileName.substr(0, fileName.length() - 1);
        }

        SYSLOG(LOG_INFO) << "Loading... ";

        Vim::Scene vimScene;
        Vim::VimErrorCodes vimLoadResult;
        try
        {
            vimLoadResult = vimScene.ReadFile(fileName);
            SYSLOG(LOG_INFO) << endl;
        }
        catch (std::exception& e)
        {
            SYSLOG(LOG_ERR) << "Error reading file " << fileName << endl;
            SYSLOG(LOG_ERR) << std::string(e.what()) << endl;
            SYSLOG(LOG_INFO) << endl;
            return ErrorCodes::Failed;
        }

        if (vimLoadResult != Vim::VimErrorCodes::Success)
        {
            SYSLOG(LOG_ERR) << "Failed to load Vim '" << fileName << endl;

            switch (vimLoadResult)
            {
            case Vim::VimErrorCodes::NoVersionInfo:
                SYSLOG(LOG_ERR) << "No version information found." << endl;
                return ErrorCodes::VersionError_Missing;

            case Vim::VimErrorCodes::FileNotRecognized:
                SYSLOG(LOG_ERR) << "Vim file not recognized." << endl;
                return ErrorCodes::FileNotRecognized;

            case Vim::VimErrorCodes::GeometryLoadingException:
                SYSLOG(LOG_ERR) << "Geometry loading exception." << endl;
                return ErrorCodes::GeometryLoadingException;

            case Vim::VimErrorCodes::AssetLoadingException:
                SYSLOG(LOG_ERR) << "Asset loading exception." << endl;
                return ErrorCodes::GeometryLoadingException;

            case Vim::VimErrorCodes::EntityLoadingException:
                SYSLOG(LOG_ERR) << "Entity loading exception." << endl;
                return ErrorCodes::EntityLoadingException;

            case Vim::VimErrorCodes::Failed:
            default:
                SYSLOG(LOG_ERR) << "Unknown failure." << endl;
                return ErrorCodes::Failed;
            }
        }

        if (vimScene.mVersionMajor > mSupportedVimVersion)
        {
            SYSLOG(LOG_ERR) << "Failed to load Vim '" << fileName << endl;
            SYSLOG(LOG_ERR) << "Attempted to load a newer unsupported version of the VIM file format (" << vimScene.mVersionMajor << "." << vimScene.mVersionMinor << "." << vimScene.mVersionPatch << ")." << endl;
            SYSLOG(LOG_ERR) << "Please upgrade to the most recent version of VIM." << endl;
            return ErrorCodes::VersionError_Unsupported;
        }

        std::unordered_map<uint32_t, VimMaterial> materialMap;

        if (!BuildMaterialMap(vimScene, materialMap))
        {
            SYSLOG(LOG_ERR) << "Failed to build material map.'" << fileName << endl;
        }

        VimSceneData* newVimSceneData = tracked_new VimSceneData();

        if (!ProcessGeometry(vimScene, materialMap, newVimSceneData))
        {
            SYSLOG(LOG_ERR) << "Failed to process vim geometry.'" << fileName << endl;
            return ErrorCodes::Failed;
        }

        if (!ProcessInstances(vimScene, scene, newVimSceneData, transform.Transposed()))
        {
            SYSLOG(LOG_ERR) << "Failed to process vim instances.'" << fileName << endl;
            return ErrorCodes::Failed;
        }

        scene->mLoadedVims.push_back(newVimSceneData);
        *outVimSceneData = newVimSceneData;

        SYSLOG(LOG_INFO) << outdent << "Done." << endl;
        return ErrorCodes::Success;
    }
}
