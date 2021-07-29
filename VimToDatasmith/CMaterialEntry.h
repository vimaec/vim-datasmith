// Copyright (c) 2021 VIM
// Licensed under the MIT License 1.0

#include "CConvertVimToDatasmith.h"
#include "CVimToDatasmith.h"

namespace Vim2Ds {

// Material's collected informations
class CVimToDatasmith::CMaterialEntry {
  public:
    // Constructor
    CMaterialEntry(MaterialId inVimId)
    : mCount(0)
    , mVimId(inVimId)
    , mMaterialElement(FDatasmithSceneFactory::CreateUEPbrMaterial(UTF8_TO_TCHAR(Utf8StringFormat("%u", inVimId).c_str()))) {}

    // Copy constructor required to be in a std::vector
    CMaterialEntry(const CMaterialEntry& inOther)
    : mCount((int32_t)inOther.mCount)
    , mVimId(inOther.mVimId)
    , mMaterialElement(inOther.mMaterialElement)
    , mColor(inOther.mColor)
    , mParams(inOther.mParams)
    , mTexture(inOther.mTexture) {}

    std::atomic<int32_t> mCount; // Number of mesh using this materials
    MaterialId mVimId; // Vim material id
    cVec4 mColor; // Vim color
    cVec2 mParams; // Vim glossiness and smoothness
    CTextureEntry* mTexture = nullptr;

    TSharedRef<IDatasmithUEPbrMaterialElement> mMaterialElement; // Datasmith version of the materials
};

} // namespace Vim2Ds
