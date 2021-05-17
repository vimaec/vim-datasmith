#ifndef __libEngine_SceneRenderer_VertexBufferData_h__
#define __libEngine_SceneRenderer_VertexBufferData_h__

#include "libEngine/Platform/Platform.h"
#include "libEngine/Math/cVec.h"
#include "libEngine/Math/cAABB.h"

namespace libEngine
{
    class VertexBufferData
    {
    public:
        uint32_t* mIndices = nullptr;
        cVec3* mPositions = nullptr;
        ubyte4* mColors = nullptr;
        ubyte4* mMaterialParams = nullptr;
        byte4* mNormals = nullptr;
        uint32_t* mObjectIds = nullptr;
        cAABB mLocalAABB = cAABB(cVec3(FLT_MAX), -cVec3(FLT_MAX));

        uint32_t mVertexCount = 0;
        uint32_t mIndexCount = 0;

    public:
        ~VertexBufferData()
        {
            delete[] mIndices;
            delete[] mPositions;
            delete[] mNormals;
            delete[] mColors;
            delete[] mObjectIds;
            delete[] mMaterialParams;
        }

        void GenerateNormals(bool flip = false);
    };
}


#endif
