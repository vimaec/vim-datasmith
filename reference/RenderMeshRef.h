#ifndef __libEngine_SceneRenderer_RenderMeshRef_h__
#define __libEngine_SceneRenderer_RenderMeshRef_h__

#include "libEngine/Math/cAABB.h"

namespace libEngine
{
    // Reference to a mesh that was registered with the renderer
    class RenderMeshRef
    {
    public:
        cAABB mAABB;
        uint32_t mStartIndex = 0;
        uint32_t mIndexCount = 0;
        uint32_t mStartVertex = 0;
        uint32_t mVertexCount = 0;
    };
}

#endif
