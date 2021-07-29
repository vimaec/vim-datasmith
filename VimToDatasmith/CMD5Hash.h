// Copyright (c) 2021 VIM
// Licensed under the MIT License 1.0

#pragma once

#include "VimToDatasmith.h"

DISABLE_SDK_WARNINGS_START

#include "Guid.h"
#include "SecureHash.h"

DISABLE_SDK_WARNINGS_END

namespace Vim2Ds {

class CMD5Hash {
  public:
    CMD5Hash() {
        m[0] = 0;
        m[1] = 0;
    }

    CMD5Hash(size_t v1, size_t v2) {
        m[0] = v1;
        m[1] = v2;
    }

    CMD5Hash(const FMD5Hash& v) { memcpy(&m, v.GetBytes(), sizeof(m)); }

    CMD5Hash(FMD5* inMD5) { inMD5->Final(reinterpret_cast<uint8*>(m)); }

    CMD5Hash CombineWith(const CMD5Hash& inOther) { return CMD5Hash(m[0] ^ inOther.m[0], m[1] ^ inOther.m[1]); }

    FString ToString() const { return ((const FGuid*)m)->ToString(); }

    bool operator==(const CMD5Hash& inOther) const { return m[0] == inOther.m[0] && m[1] == inOther.m[1]; }

    struct SHasher {
        std::size_t operator()(const CMD5Hash& v) const { return v.m[0] ^ v.m[1]; }
    };

  private:
    size_t m[2];
};

} // namespace Vim2Ds
