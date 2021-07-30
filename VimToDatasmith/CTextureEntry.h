// Copyright (c) 2021 VIM
// Licensed under the MIT License 1.0

#include "CConvertVimToDatasmith.h"
#include "CVimToDatasmith.h"

namespace Vim2Ds {

// Material's collected informations
class CVimToDatasmith::CTextureEntry {
  public:
    // Constructor
    CTextureEntry(CVimToDatasmith* inVimToDatasmith, const bfast::Buffer& inImageBuffer)
    : mVimToDatasmith(inVimToDatasmith)
    , mImageBuffer(inImageBuffer) {
        // Name of texture is the content.
        FMD5 MD5;
        MD5.Update(mImageBuffer.data.begin(), mImageBuffer.data.size());
        CMD5Hash MD5Hash(&MD5);
        mDatasmithName = MD5Hash.ToString();
        mDatasmithLabel = UTF8_TO_TCHAR(mImageBuffer.name.c_str());
    }

    const TCHAR* GetName() const { return *mDatasmithName; }

    const TCHAR* GetLabel() const { return *mDatasmithLabel; }

    // Copy the texture in the asset texture's folder
    void CopyTextureInAssets() {
        if (!CreateFolder(TCHAR_TO_UTF8(*mVimToDatasmith->mConverter.GetOutputPath())))
            return;
        FString textureFolderPath = mVimToDatasmith->mConverter.GetOutputPath() + TEXT("/Textures");
        if (!CreateFolder(TCHAR_TO_UTF8(*textureFolderPath)))
            return;

        FString filePathName = textureFolderPath + TEXT("/") + mDatasmithName;
        size_t posExtension = mImageBuffer.name.find_last_of('.');
        if (posExtension != std::string::npos)
            filePathName += UTF8_TO_TCHAR(mImageBuffer.name.c_str() + posExtension);
        else {
            DebugF("CVimToDatasmith::CTextureEntry::CopyTextureInAssets - Texture file \"%s\" hasn't extension\n", mImageBuffer.name.c_str());
            return;
        }

#ifndef __clang__
        FILE* file = nullptr;
        if (_wfopen_s(&file, *filePathName, TEXT("wb")) != 0)
            file = nullptr;
#else
        FILE* file = fopen(TCHAR_TO_UTF8(*filePathName), "wb");
#endif
        if (file != nullptr) {
            if (fwrite(mImageBuffer.data.begin(), mImageBuffer.data.size(), 1, file) != 1)
                DebugF("CVimToDatasmith::CTextureEntry::CopyTextureInAssets - Can't write to file \"%s\" - error=%d\n", TCHAR_TO_UTF8(*filePathName), errno);
            fclose(file);
        } else
            DebugF("CVimToDatasmith::CTextureEntry::CopyTextureInAssets - Can't open file \"%s\" for writing - error=%d\n", TCHAR_TO_UTF8(*filePathName),
                   errno);
    }

    // Add a texture element to the scne
    void AddToScene() {
        if (!mDatasmithTexture.IsValid()) {
            CopyTextureInAssets();
            mDatasmithTexture = FDatasmithSceneFactory::CreateTexture(*mDatasmithName);
            size_t posExtension = mImageBuffer.name.find_last_of('.');
            const char* extension = ".png";
            EDatasmithTextureFormat fmt = EDatasmithTextureFormat::PNG;
            if (posExtension != std::string::npos) {
                extension = mImageBuffer.name.c_str() + posExtension;
                if (strcmp(extension, ".jpg") == 0)
                    fmt = EDatasmithTextureFormat::JPEG;
            }

            FString filePathName = mVimToDatasmith->mConverter.GetOutputPath() + TEXT("/Textures/") + mDatasmithName;
            filePathName += UTF8_TO_TCHAR(extension);
            mDatasmithTexture->SetFile(*filePathName);
            FMD5Hash FileHash = FMD5Hash::HashFile(mDatasmithTexture->GetFile());
            mDatasmithTexture->SetFileHash(FileHash);

            mDatasmithTexture->SetLabel(*mDatasmithLabel);
            mDatasmithTexture->SetSRGB(EDatasmithColorSpace::sRGB);
            std::unique_lock<std::mutex> lk(mVimToDatasmith->mConverter.GetSceneAccess());
            mVimToDatasmith->mConverter.GetScene()->AddTexture(mDatasmithTexture);
        }
    }

  private:
    CVimToDatasmith* const mVimToDatasmith;
    const bfast::Buffer& mImageBuffer;
    FString mDatasmithName;
    FString mDatasmithLabel;
    TSharedPtr<IDatasmithTextureElement> mDatasmithTexture;
};

} // namespace Vim2Ds
