// Copyright (c) 2021 VIM
// Licensed under the MIT License 1.0

#include "CConvertVimToDatasmith.h"
#include "CVimToDatasmith.h"

DISABLE_SDK_WARNINGS_START

#include "DatasmithExporterManager.h"
#include "DatasmithMesh.h"
#include "DatasmithMeshExporter.h"
#include "DatasmithSceneExporter.h"
#include "DatasmithSceneFactory.h"

#include "Math/Transform.h"
#include "Paths.h"

DISABLE_SDK_WARNINGS_END

namespace Vim2Ds {

CConvertVimToDatasmith::CConvertVimToDatasmith() {
}

CConvertVimToDatasmith::~CConvertVimToDatasmith() {
}

// Parse parameters to get Vim file path and datasmith file path
void CConvertVimToDatasmith::GetParameters(int argc, const utf8_t* const* argv) {
    if (argc > 1 && argv[1][0] == '-') {
        if (strcmp(argv[1], "-NoHierarchicalInstance") == 0) {
            mNoHierarchicalInstance = true;
            --argc;
            ++argv;
        }
    }

    if (argc < 2 || argc > 3)
        Usage();

    // Process input vim file argument
    mVimFilePath = argv[1];

    std::string vimPath;
    std::string vimName;
    std::string vimExtension;
    ExtractPathNameExtension(mVimFilePath, &vimPath, &vimName, &vimExtension);

    if (vimExtension != ".vim")
        ThrowMessage("Invalid vim file name \"%s\"", mVimFilePath.c_str());

    // Process output datassmith file argument
    if (argc > 2) {
        std::string datasmithExtension;
        ExtractPathNameExtension(argv[2], &mDatasmithFolderPath, &mDatasmithFileName, &datasmithExtension);
        if (datasmithExtension.size() != 0 && datasmithExtension != ".udatasmith")
            ThrowMessage("Invalid datasmith file name \"%s\"", argv[2]);
        if (mDatasmithFileName.size() == 0)
            mDatasmithFileName = vimName;
    } else {
        mDatasmithFolderPath = vimPath;
        mDatasmithFileName = vimName;
    }

    mOutputPath = UTF8_TO_TCHAR((mDatasmithFolderPath + "/" + mDatasmithFileName + "_Assets").c_str());

    DebugF("Convert \"%s\" -> \"%s\"\n", mVimFilePath.c_str(), (mDatasmithFolderPath + "/" + mDatasmithFileName + ".udatasmith").c_str());
}

void CConvertVimToDatasmith::Convert() {
    mTotalTimeStat.BeginNow();
    CTaskMgr::CTaskJointer jointer("CConvertVimToDatasmith::Convert");

    CreateScene();

    ReadVimFile();
    mVimTodatasmith.reset(new CVimToDatasmith(this));

    mVimPrepareStat.BeginNow();
    (new CTaskMgr::TJoinableFunctorTask<CVimImported*>([](CVimImported* inVimImporter) { inVimImporter->Prepare(); }, mVim.get()))->Start(&jointer);
    (new CTaskMgr::TJoinableFunctorTask<CVimToDatasmith*>([](CVimToDatasmith* inVimTodatasmith) { inVimTodatasmith->CreateMaterials(); },
                                                          mVimTodatasmith.get()))
        ->Start(&jointer);
    jointer.Join();
    mVimPrepareStat.FinishNow();

    mVimTodatasmith->ConvertScene();

    // Deletion, validation and writing
    (new CTaskMgr::TJoinableFunctorTask<CConvertVimToDatasmith*>([](CConvertVimToDatasmith* me) { me->mVimTodatasmith.reset(); }, this))->Start(&jointer);
    (new CTaskMgr::TJoinableFunctorTask<CConvertVimToDatasmith*>([](CConvertVimToDatasmith* me) { me->mVim.reset(); }, this))->Start(&jointer);
    (new CTaskMgr::TJoinableFunctorTask<CConvertVimToDatasmith*>(
         [](CConvertVimToDatasmith* me) {
             me->Validate();
             me->CreateDatasmithFile();
         },
         this))
        ->Start(&jointer);
    jointer.Join();

    mTotalTimeStat.FinishNow();
    ReportTimeStat();
}

void CConvertVimToDatasmith::CreateScene() {
    mDatasmithScene = FDatasmithSceneFactory::CreateScene(UTF8_TO_TCHAR(mVimFilePath.c_str()));
    mDatasmithScene->SetHost(TEXT("Vim"));
    mDatasmithScene->SetVendor(TEXT("VIMaec"));
    mDatasmithScene->SetProductName(TEXT("VimToDatasmith"));
    mDatasmithScene->SetProductVersion(UTF8_TO_TCHAR("1.0.0"));
}

void CConvertVimToDatasmith::ReadVimFile() {
    mVimLoadStat.BeginNow();
    mVim.reset(new CVimImported());
    mVim->Read(mVimFilePath);
    mVimLoadStat.FinishNow();
}

// Write a Datasmith scene to the Datasmith file
void CConvertVimToDatasmith::CreateDatasmithFile() {
    VerboseF("CVimToDatasmith::CreateDatasmithFile\n");
    mWriteTimeStat.BeginNow();

    FDatasmithExportOptions::PathTexturesMode = EDSResizedTexturesPath::OriginalFolder;
    FDatasmithSceneExporter SceneExporter;

    SceneExporter.PreExport();
    SceneExporter.SetName(UTF8_TO_TCHAR(mDatasmithFileName.c_str()));

    SceneExporter.SetOutputPath(UTF8_TO_TCHAR(mDatasmithFolderPath.c_str()));

    SceneExporter.Export(mDatasmithScene.ToSharedRef());
    mWriteTimeStat.FinishNow();
}

#if UseValidator
// For Datasmith::FSceneValidator::PrintReports
static void Trace(const utf8_t* FormatString, ...) {
    va_list argptr;
    va_start(argptr, FormatString);
    vfprintf(stderr, FormatString, argptr);
    va_end(argptr);
}
#endif

void CConvertVimToDatasmith::Validate() {
#if UseValidator
    mValidationTimeStat.BeginNow();
    Datasmith::FSceneValidator validator(mDatasmithScene.ToSharedRef());
    validator.CheckElementsName();
    validator.CheckDependances();
    validator.PrintReports(Datasmith::FSceneValidator::kVerbose, Trace);
    mValidationTimeStat.FinishNow();
#endif
}

// Print time statistics
void CConvertVimToDatasmith::ReportTimeStat() {
    EP2DB tmp = SetPrintLevel(kP2DB_Trace);
    mTotalTimeStat.PrintTime("Total", kP2DB_Report);
    mVimLoadStat.PrintTime("Load");
    mVimPrepareStat.PrintTime("Prepare");
    mBuildMeshTimeStat.PrintTime("Mesh");
    mBuildMetaDataTimeStat.PrintTime("MetaData");
    mBuildMetaDataTimeStat.PrintTime("Tags");
    mValidationTimeStat.PrintTime("Validation");
    mWriteTimeStat.PrintTime("Write");
    SetPrintLevel(tmp);
}

} // namespace Vim2Ds
