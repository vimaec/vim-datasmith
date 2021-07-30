// Copyright (c) 2021 VIM
// Licensed under the MIT License 1.0

#pragma once

#include "VimToDatasmith.h"

DISABLE_SDK_WARNINGS_START

#include "vim.h"

DISABLE_SDK_WARNINGS_END

namespace Vim2Ds {

// Basic vector class
template <class C, class Indexor = std::size_t> class TVector {
  public:
    virtual ~TVector(){};

    // Return the size
    Indexor Count() const { return Indexor(mCount); }

    // Access to element  by it's index
    const C& operator[](Indexor inIndex) const {
        TestAssertDebugOnly(inIndex < mCount);
        return mData[inIndex];
    }

    // Access to element  by it's index
    C& operator[](Indexor inIndex) {
        TestAssertDebugOnly(mData != nullptr && inIndex < mCount);
        return mData[inIndex];
    }

    // Print the content of this vector
    void Print(const utf8_t* inDataName, const utf8_t* inSeparator = "\n\t\t") {
        TraceF("\t%s Count=%lu", inDataName, mCount);
        for (Indexor index = Indexor(0); index < mCount; Increment(index))
            TraceF("%s%lu %s", inSeparator, index, ToUtf8(mData[index]));
        TraceF("\n");
    }

    const C* begin() const { return mData; }

    const C* end() const { return mData + mCount; }

  protected:
    size_t mCount = 0; // Return the number of elements
    C* mData = nullptr; // Array of the elements
};

// Vector class that refers to attribute content.
/* Doesn't copy the attribute's content */
template <class C, class Indexor = std::size_t> class TAttributeVector : public TVector<C, Indexor> {
  public:
    TAttributeVector() {}
    TAttributeVector(const g3d::Attribute& inAttr) { Initialize(inAttr); }

    void Initialize(const g3d::Attribute& inAttr) {
        size_t dataSize = inAttr.byte_size();
        this->mCount = dataSize / sizeof(C);
        TestAssert(dataSize == this->mCount * sizeof(C));
        this->mData = reinterpret_cast<C*>(inAttr._begin);
    }
};

// Vector class that refers to vector of integer.
/* Doesn't copy the vector */
template <class C, class Indexor> class TIndexor : public TVector<C, Indexor> {
    static_assert(sizeof(C) == sizeof(int), "C hasn't same size as int");

  public:
    TIndexor() {}
    TIndexor(const std::vector<int>& inOriginal) { Initialize(inOriginal); }

    void Initialize(const std::vector<int>& inOriginal) {
        this->mCount = inOriginal.size();
        if (this->mCount > 0)
            this->mData = const_cast<C*>(reinterpret_cast<const C*>(&inOriginal[0]));
    }
};

// Vector that allocate it's content
template <class C, class Indexor = std::size_t> class TAllocatedVector : public TVector<C, Indexor> {
  public:
    TAllocatedVector() {}
    TAllocatedVector(Indexor inCount) { Allocate(inCount); }

    // Destructor
    ~TAllocatedVector() { Clear(); }

    // Unallocate
    void Clear() {
        this->mCount = 0;
        delete[] this->mData;
        this->mData = nullptr;
    }

    // Allocate the required number of elements
    void Allocate(Indexor inCount) {
        TestAssert(this->mData == nullptr);
        this->mCount = inCount;
        this->mData = new C[this->mCount]{};
    }
};

} // namespace Vim2Ds
