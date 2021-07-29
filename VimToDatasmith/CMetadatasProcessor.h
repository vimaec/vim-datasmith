// Copyright (c) 2021 VIM
// Licensed under the MIT License 1.0

#include "CVimToDatasmith.h"

namespace Vim2Ds {

class CVimToDatasmith::CMetadatasProcessor {
  public:
    CMetadatasProcessor(CVimToDatasmith* inVimTodatasmith)
    : mVimTodatasmith(inVimTodatasmith)
    , mProperties(inVimTodatasmith->mVim.GetEntitiesTable("table:Rvt.Element").mProperties) {
        mStart = mProperties.size() > 0 ? &mProperties[0] : nullptr;
        mEnd = mStart + mProperties.size();
    }

    void Process() {
        CTaskMgr::CTaskJointer createAllMetaDatas("CreateAllMetaDatas");
        for (int i = 0; i < CTaskMgr::Get().GetNbProcessors(); ++i)
            (new CTaskMgr::TJoinableFunctorTask<CMetadatasProcessor*>([](CMetadatasProcessor* me) { me->Proceed(); }, this))->Start(&createAllMetaDatas);
        createAllMetaDatas.Join();
    }

  private:
    void Proceed() {
        const Vim::SerializableProperty* start;
        const Vim::SerializableProperty* end;
        CVimToDatasmith::CActorEntry* actorEntry = GetObject(&start, &end);
        while (actorEntry != nullptr) {
            while (start < end) {
                TSharedPtr<IDatasmithKeyValueProperty> dsProperty =
                    FDatasmithSceneFactory::CreateKeyValueProperty(UTF8_TO_TCHAR(mVimTodatasmith->mVim.GetString(StringIndex(start->mName))));
                dsProperty->SetValue(UTF8_TO_TCHAR(mVimTodatasmith->mVim.GetString(StringIndex(start->mValue))));
                dsProperty->SetPropertyType(EDatasmithKeyValuePropertyType::String);
                actorEntry->GetOrCreateMetadataElement(mVimTodatasmith).AddProperty(dsProperty);
                ++start;
            }
            actorEntry = GetObject(&start, &end);
        }
    }

    CVimToDatasmith::CActorEntry* GetObject(const Vim::SerializableProperty** outStart, const Vim::SerializableProperty** outEnd) {
        std::unique_lock<std::mutex> lk(mAccessControl);
        while (mStart < mEnd) {
            ElementIndex elementIndex = ElementIndex(mStart->mEntityId);
            *outStart = mStart;
            ++mStart;
            if (elementIndex < mVimTodatasmith->mVecElementToActors.size()) {
                CVimToDatasmith::CActorEntry& actorEntry = mVimTodatasmith->mVecElementToActors[elementIndex];
                if (actorEntry.HasElement()) {
                    while (mStart < mEnd && ElementIndex(mStart->mEntityId) == elementIndex)
                        ++mStart;
                    *outEnd = mStart;
                    return &actorEntry;
                }
            }
        }
        *outStart = nullptr;
        *outEnd = nullptr;
        return nullptr;
    }

    std::mutex mAccessControl;
    const Vim::SerializableProperty* mStart;
    const Vim::SerializableProperty* mEnd;
    CVimToDatasmith* const mVimTodatasmith;
    const std::vector<Vim::SerializableProperty>& mProperties;
};

} // namespace Vim2Ds
