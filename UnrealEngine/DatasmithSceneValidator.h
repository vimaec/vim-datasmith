// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "VimToDsWarningsDisabler.h"

DISABLE_SDK_WARNINGS_START
#include "IDatasmithSceneElements.h"
DISABLE_SDK_WARNINGS_END

#include "Array.h"
#include "Map.h"

namespace Datasmith {

class FNamePtr {
  public:
    FNamePtr() {}

    FNamePtr(const TCHAR* InName)
    : Name(InName) {}

    bool operator<(const FNamePtr& InOther) const { return FCString::Strcmp(Name, InOther.Name) < 0; }

    bool operator==(FNamePtr InOther) const { return FCString::Strcmp(Name, InOther.Name) == 0; }

    const TCHAR* Name = nullptr;
};

inline uint32 GetTypeHash(const FNamePtr& A) {
    return FCrc::Strihash_DEPRECATED(FCString::Strlen(A.Name), A.Name);
}

class FSceneValidator {
  public:
    enum TInfoLevel { kBug, kError, kWarning, kVerbose, kInfoLevelMax };
    static const char* LevelName(TInfoLevel Level);

    FSceneValidator(const TSharedRef<IDatasmithScene>& InScene);

    TSharedRef<IDatasmithScene> Scene;

    FString GetElementTypes(const IDatasmithElement& InElement);

    FString GetElementsDescription(const IDatasmithElement& InElement);

    void CheckElementsName();

    void CheckActorsName(const IDatasmithActorElement& InActor);

    void CheckDependances();

    void CheckActorsDependances(const IDatasmithActorElement& InActor);

    typedef TMap<FNamePtr, const IDatasmithElement*> MapNameToElement;

    MapNameToElement NameToElementMap;

    class FUsage {
      public:
        bool bExist = false;
        bool bIsRefered = false;
    };

    typedef TMap<FNamePtr, FUsage> FMapNameToUsage;

    FMapNameToUsage TexturesUsages;
    FMapNameToUsage MaterialsUsages;
    FMapNameToUsage MeshesUsages;
    FMapNameToUsage ActorsUsages;
    FMapNameToUsage LevelSequencesUsages;

    void AddElements(const IDatasmithElement& InElement, FMapNameToUsage* IOMap = nullptr);

    template <typename FmtType, typename... Types> void AddMessage(TInfoLevel Level, const FmtType& Fmt, Types... Args) {
        AddMessageImpl(Level, FString::Printf(Fmt, Args...));
    }

    void AddMessageImpl(TInfoLevel Level, const FString& Message);

    typedef void (*FctReport)(const char* inUtf8Format, ...);
    void PrintReports(TInfoLevel Level, FctReport InFctReport);

    class FMessage {
      public:
        FMessage(TInfoLevel InLevel, const FString& InMessage)
        : Level(InLevel)
        , Message(InMessage) {}

        TInfoLevel Level;
        FString Message;
    };

    TArray<FMessage> Messages;

    uint32_t MessagesCounts[kInfoLevelMax] = {};
};

} // namespace Datasmith
