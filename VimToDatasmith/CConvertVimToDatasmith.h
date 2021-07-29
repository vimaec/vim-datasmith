// Copyright (c) 2021 VIM
// Licensed under the MIT License 1.0

#pragma once

#include "TimeStat.h"
#include "VimToDatasmith.h"

DISABLE_SDK_WARNINGS_START

#include "IDatasmithSceneElements.h"

DISABLE_SDK_WARNINGS_END

#include <mutex>

namespace Vim2Ds {

class CVimImported;
class CVimToDatasmith;

class CConvertVimToDatasmith {
  public:
    CConvertVimToDatasmith();
    ~CConvertVimToDatasmith();

    // Parse parameters to get Vim file path and datasmith file path
    void GetParameters(int argc, const utf8_t* const* argv);

    void Convert();

    CVimImported& GetVim() const { return *mVim; }

    CVimToDatasmith& GetDatasmith() const { return *mVimTodatasmith; }

    const TSharedPtr<IDatasmithScene>& GetScene() const { return mDatasmithScene; }
    std::mutex& GetSceneAccess() { return mDatasmithSceneAccessControl; }

    const FString& GetOutputPath() const { return mOutputPath; }

    bool GetNoHierarchicalInstance() const { return mNoHierarchicalInstance; }

    FTimeStat mBuildMetaDataTimeStat;
    FTimeStat mBuildTagsTimeStat;

  private:
    void CreateScene();
    void ReadVimFile();
    void Validate();
    void CreateDatasmithFile();
    void ReportTimeStat();

    // Extracted from parameters
    bool mNoHierarchicalInstance = false;
    std::string mVimFilePath;
    std::string mDatasmithFolderPath;
    std::string mDatasmithFileName;
    FString mOutputPath;

    std::unique_ptr<CVimImported> mVim;
    std::unique_ptr<CVimToDatasmith> mVimTodatasmith;

    // Datasmith scene and assets output path
    TSharedPtr<IDatasmithScene> mDatasmithScene;
    std::mutex mDatasmithSceneAccessControl;

    FTimeStat mTotalTimeStat;
    FTimeStat mVimLoadStat;
    FTimeStat mVimPrepareStat;
    FTimeStat mBuildMeshTimeStat;
    FTimeStat mValidationTimeStat;
    FTimeStat mWriteTimeStat;
};

} // namespace Vim2Ds
