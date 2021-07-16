#ifndef __Utils_h__
#define __Utils_h__

#include "Platform.h"
#include <malloc/malloc.h>
#include <vector>
#include <string>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional> // std::reference_wrapper, std::ref

#define elementsof(arr) sizeof(Utils::ArrayCountHelper(arr))
#define sizeof_array(arr) sizeof(Utils::ArraySizeHelper(arr))

#define ENUM_OPERATORS(TYPE, ENUM_TYPE)                                                                        \
    static inline ENUM_TYPE operator|(ENUM_TYPE a, ENUM_TYPE b) { return (ENUM_TYPE)((TYPE)(a) | (TYPE)(b)); } \
    static inline ENUM_TYPE operator&(ENUM_TYPE a, ENUM_TYPE b) { return (ENUM_TYPE)((TYPE)(a) & (TYPE)(b)); } \
    static inline ENUM_TYPE operator|=(ENUM_TYPE& a, ENUM_TYPE b) { return a = (a | b); }                      \
    static inline ENUM_TYPE operator&=(ENUM_TYPE& a, ENUM_TYPE b) { return a = (a & b); }

namespace Utils
{
    template <typename T, size_t N> char(&ArrayCountHelper(T(&arr)[N]))[N];
    template <typename T, size_t N> T(&ArraySizeHelper(T(&arr)[N]))[N];
    // These must be force_inline so that the allocation happens in the caller's stack frame
    template <typename T> __forceinline T* StackAlloc(size_t N) { return (T*)alloca(N * sizeof(T)); }
    template <typename T> __forceinline T* StackAllocZeroed(size_t N) { void* mem = alloca(N * sizeof(T)); memset(mem, 0, N * sizeof(T)); return (T*)mem; }

    // Note: returns true for 0
    inline bool IsPowerOf2(const int x)
    {
        return (x & (x - 1)) == 0;
    }

    template <typename T0, typename T1>
    inline T0 Align(const T0 x, const T1 a)
    {
        return (x + (a - 1)) & (T0)(~(a - 1));
    }

    template <typename T>
    inline T Min(const T t0, const T t1)
    {
        return (t0 < t1) ? t0 : t1;
    }

    template <typename T>
    inline T Max(const T t0, const T t1)
    {
        return (t0 < t1) ? t1 : t0;
    }

    template <typename T>
    inline T Clamp(const T t, const T s0, const T s1)
    {
        return Min(Max(t, s0), s1);
    }

    template <typename T>
    inline T Saturate(const T t)
    {
        return Clamp<T>(t, (T)0, (T)1);
    }

    template <uint8_t a, uint8_t b, uint8_t c, uint8_t d>
    struct MCHAR4
    {
        static const uint32_t V = (a | (b << 8) | (c << 16) | (d << 24));
    };

    template <uint8_t a, uint8_t b>
    struct MCHAR2
    {
        static const uint32_t V = (a | (b << 8));
    };

    inline const char** CreateCStringArray(std::vector<std::string>& stringArray)
    {
        const char** ret = tracked_new const char* [stringArray.size()];
        for (int i = 0; i < stringArray.size(); i++)
        {
            ret[i] = stringArray[i].c_str();
        }

        return ret;
    }

    template <typename T>
    inline T RoundUp(T value, T multiple)
    {
        return ((value + multiple - 1) / multiple) * multiple; 
    }

    template <typename T>
    inline T RoundDown(T value, T multiple)
    {
        return value - value % multiple; 
    }

    extern const char* ReadFile(const std::string& fileName, off_t* size = nullptr);


    /*
    Returns true if given number is a power of two.
    T must be unsigned integer number or signed integer but always nonnegative.
    For 0 returns true.
    */
    template <typename T>
    inline bool IsPow2(T x)
    {
        return (x & (x - 1)) == 0;
    }

    // Returns smallest power of 2 greater or equal to v.
    static inline uint32_t NextPow2(uint32_t v)
    {
        v--;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        v++;
        return v;
    }
    static inline uint64_t NextPow2(uint64_t v)
    {
        v--;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        v |= v >> 32;
        v++;
        return v;
    }

    // Returns largest power of 2 less or equal to v.
    static inline uint32_t PrevPow2(uint32_t v)
    {
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        v = v ^ (v >> 1);
        return v;
    }
    static inline uint64_t PrevPow2(uint64_t v)
    {
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        v |= v >> 32;
        v = v ^ (v >> 1);
        return v;
    }

    template <typename t>
    class SafeVector
    {
        const std::vector<t>& mVector;
        const t mDefault;
    public:
        SafeVector(const std::vector<t>& vector, const t deflt) :
            mVector(vector),
            mDefault(deflt)
        {}

        t& operator [] (int index)
        {
            if (index >= 0 && index < mVector.size())
            {
                return mVector[index];
            }

            return mDefault;
        }
    };

    class OrderedLock
    {
        std::queue<std::reference_wrapper<std::condition_variable>> mConditionVar;
        std::mutex mMutex;
        bool mLocked;
    public:
        OrderedLock() :
            mLocked(false)
        {}

        void lock()
        {
            std::unique_lock<std::mutex> acquire(mMutex);
            if (mLocked)
            {
                std::condition_variable signal;
                mConditionVar.emplace(std::ref(signal));
                signal.wait(acquire);
                mConditionVar.pop();
            }
            else
            {
                mLocked = true;
            }
        }
        void unlock()
        {
            std::unique_lock<std::mutex> acquire(mMutex);
            if (mConditionVar.empty())
            {
                mLocked = false;
            }
            else
            {
                mConditionVar.front().get().notify_one();
            }
        }
    };
}
#endif
