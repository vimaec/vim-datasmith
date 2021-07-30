// Copyright Epic Games, Inc. All Rights Reserved.

#include "TimeStat.h"

#if macOS
#include <sys/time.h>
#include <sys/times.h>
#include <unistd.h>
#endif
#if winOS
#include <windows.h>
#endif

namespace Vim2Ds {

// Reset to current process CPU time and real time
void FTimeStat::BeginNow() {
    mStartCpuTime = CpuTimeClock();
    mStartRealTime = RealTimeClock();
    mFinishCpuTime = 0.0;
    mFinishRealTime = 0.0;
}

// Finish to current process CPU time and real time
void FTimeStat::FinishNow() {
    mFinishCpuTime = CpuTimeClock();
    mFinishRealTime = RealTimeClock();
}

// Cumulate time from the other
void FTimeStat::AddDiff(const FTimeStat& InOther) {
    mStartCpuTime -= InOther.GetCpuTime();
    mStartRealTime -= InOther.GetRealTime();
}

// Print time differences
void FTimeStat::PrintTime(const char* InStatLabel, EP2DB inMsgLevel) {
    double CpuSeconds = mFinishCpuTime - mStartCpuTime;
    double RealSeconds = GetRealTime();
    const char* warning = "";
    if (mStartCpuTime != 0.0) {
        if (CpuSeconds < 0.0) {
            CpuSeconds = CpuTimeClock() - mStartCpuTime;
            RealSeconds = RealTimeClock() - mStartRealTime;
            warning = "*** FTimeStat::FinishNow hasn't been called *** ";
        }

        if (RealSeconds >= 100.0)
            Vim2Ds::Printf2DB(inMsgLevel, "%sSeconds for %s cpu=%.0lfs, real=%.0lfs\n", warning, InStatLabel, CpuSeconds, RealSeconds);
        else if (CpuSeconds >= 100.0)
            Vim2Ds::Printf2DB(inMsgLevel, "%sSeconds for %s cpu=%.0lfs, real=%.2lgs\n", warning, InStatLabel, CpuSeconds, RealSeconds);
        else
            Vim2Ds::Printf2DB(inMsgLevel, "%sSeconds for %s cpu=%.2lgs, real=%.2lgs\n", warning, InStatLabel, CpuSeconds, RealSeconds);
    } else if (mFinishCpuTime != 0.0)
        Vim2Ds::Printf2DB(inMsgLevel, "FTimeStat::PrintTime - BeginNow() hasn't been called for stat %s\n", InStatLabel);
}

// Return elapsed  real time
double FTimeStat::GetRealTime() const {
    double RealSeconds = mFinishRealTime - mStartRealTime;
    if (RealSeconds < 0) {
        // Before and after midnight ?
        RealSeconds += 24 * 60 * 60;
    }
    return RealSeconds;
}

// Tool get current real time clock
double FTimeStat::RealTimeClock() {
#if PLATFORM_WINDOWS
    LARGE_INTEGER Time, Freq;
    if (QueryPerformanceFrequency(&Freq) && QueryPerformanceCounter(&Time)) {
        return double(Time.QuadPart) / Freq.QuadPart;
    }
#else
    struct timeval Time;
    if (gettimeofday(&Time, NULL) == 0) {
        return Time.tv_sec + double(Time.tv_usec) * .000001;
    }
#endif
    return 0;
}

// Tool get process CPU real time clock
double FTimeStat::CpuTimeClock() {
#if PLATFORM_WINDOWS
    FILETIME A, B, C, D;
    if (GetProcessTimes(GetCurrentProcess(), &A, &B, &C, &D) != 0) {
        return double(D.dwLowDateTime | ((unsigned long long)D.dwHighDateTime << 32)) * 0.0000001;
    }
    return 0;
#else
    static double spc = 1.0 / sysconf(_SC_CLK_TCK);
    struct tms AllTimes;
    times(&AllTimes);
    return (AllTimes.tms_utime + AllTimes.tms_stime + AllTimes.tms_cutime + AllTimes.tms_cstime) * spc;
#endif
}

} // namespace Vim2Ds
