// Copyright (c) 2021 VIM
// Licensed under the MIT License 1.0

#include "CConvertVimToDatasmith.h"
#include "CVimToDatasmith.h"

namespace Vim2Ds {

// Material's collected informations
class CVimToDatasmith::CActorEntry {
  public:
    CActorEntry() {}

    bool HasElement() const { return mActorElement.IsValid(); }

    void SetActor(const TSharedRef<IDatasmithActorElement>& inActorElement, NodeIndex inNodeIndex) {
        if (inNodeIndex < mLowestNodeIndex)
            mActorElement = inActorElement;
        TestAssert(mActorElement.IsValid());
    }

    IDatasmithActorElement* GetActorElement() { return mActorElement.Get(); }

    IDatasmithMetaDataElement& GetOrCreateMetadataElement(CVimToDatasmith* inVimToDatasmith) {
        if (!mMetaDataElement.IsValid()) {
            TestPtr(mActorElement);
            FString metadataName(FString::Printf(TEXT("MetaData_%s"), mActorElement->GetName()));
            mMetaDataElement = FDatasmithSceneFactory::CreateMetaData(*metadataName);
            mMetaDataElement->SetAssociatedElement(mActorElement);

            std::unique_lock<std::mutex> lk(inVimToDatasmith->mConverter.GetSceneAccess());
            inVimToDatasmith->mConverter.GetScene()->AddMetaData(mMetaDataElement);
        }
        return *mMetaDataElement;
    }

    void AddTag(const utf8_t* inTag, std::vector<int>& inVector, ElementIndex inIndex);

  private:
    TSharedPtr<IDatasmithActorElement> mActorElement;
    TSharedPtr<IDatasmithMetaDataElement> mMetaDataElement;
    NodeIndex mLowestNodeIndex = NodeIndex::kNoNode;
};

} // namespace Vim2Ds
