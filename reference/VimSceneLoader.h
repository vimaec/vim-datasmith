#ifndef __libEngine_Scene_VimSceneLoader_h__
#define __libEngine_Scene_VimSceneLoader_h__

#pragma warning(disable: 4091) // Warning	C4091	'static ': ignored on left of 'XXX' when no variable is declared


#include "Utils/Utils.h"
#include "Utils/ErrorCodes.h"
#include "libEngine/SceneRenderer/VertexBufferData.h"
#include "libEngine/SceneRenderer/RenderMeshRef.h"

namespace libEngine
{
    class Scene;
    class InstanceAllocation;

    class VimSceneData
    {
    public:
        VertexBufferData mVertexBuffer;
        RenderMeshRef mMeshRef;
        float mMinHeight = 0.0f;
        float mMaxHeight = 0.0f;
        cVec3 mAABBCenter;
        cVec3 mAABBExtent;

        cAABB* mGeometryAABBMap = nullptr;
        bool* mIsAlphaInstanceMap = nullptr;
        uint32_t* mGroupIndexCounts = nullptr;
        uint32_t* mGroupVertexCounts = nullptr;
        uint32_t* mGroupIndexOffets = nullptr;

        std::vector<uint64_t> mInstances;
    };

    static class VimSceneLoader
    {
    public:
        static const uint32_t mSupportedVimVersion = 1;

        static ErrorCodes LoadVim(std::string fileName, Scene* scene, VimSceneData** outVimSceneData, cMat4 transform);
    };
}

#endif
