#include "VertexBufferData.h"

namespace libEngine
{
    void VertexBufferData::GenerateNormals(bool flip)
    {
        delete mNormals;

        cVec3* normals = tracked_new cVec3[mVertexCount];
        for (uint32_t i = 0; i < mIndexCount; i += 3)
        {
            cVec3 v0 = mPositions[mIndices[i + 0]];
            cVec3 v1 = mPositions[mIndices[i + 1]];
            cVec3 v2 = mPositions[mIndices[i + 2]];

            cVec3 s0 = v2 - v0;
            cVec3 s1 = v2 - v1;

            cVec3 normal = s1 ^ s0;
            normal.Normalise();

            normals[mIndices[i + 0]] += normal;
            normals[mIndices[i + 1]] += normal;
            normals[mIndices[i + 2]] += normal;
        }

        mNormals = tracked_new byte4[mVertexCount];
        if (flip)
        {
            for (uint32_t i = 0; i < mVertexCount; i++)
            {
                normals[i].Normalise();
                mNormals[i] = -normals[i];
            }
        }
        else
        {
            for (uint32_t i = 0; i < mVertexCount; i++)
            {
                normals[i].Normalise();
                mNormals[i] = normals[i];
            }
        }

        delete[] normals;
    }


}
