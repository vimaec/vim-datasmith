// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "DebugTools.h"
#include "VimToDsWarningsDisabler.h"

namespace Vim2Ds {

// Get cpu and real time
class FTimeStat {
  public:
    // Reset to current process CPU time and real time
    void BeginNow();

    // Finish to current process CPU time and real time
    void FinishNow();

    // Cumulate time from the other
    void AddDiff(const FTimeStat& InOther);

    // Print time differences
    void PrintTime(const char* InStatLabel, EP2DB inMsgLevel = kP2DB_Trace);

    // Return cpu time at BeginNow
    double GetStartCpuTime() const { return mStartCpuTime; }

    // Return real time at BeginNow
    double GetStartRealTime() const { return mStartRealTime; }

    // Return elapsed cpu time
    double GetCpuTime() const { return mFinishCpuTime - mStartCpuTime; }

    // Return elapsed  real time
    double GetRealTime() const;

    // Tool get current real time clock
    static double RealTimeClock();

    // Tool get process CPU real time clock
    static double CpuTimeClock();

  private:
    double mStartCpuTime = 0.0;
    double mStartRealTime = 0.0;
    double mFinishCpuTime = 0.0;
    double mFinishRealTime = 0.0;
};

// Macro to measure code execution time
#define MeasureTime(name, code, msgLevel)      \
    {                                          \
        FTimeStat name##Stat;                  \
        name##Stat.BeginNow();                 \
        code;                                  \
        name##Stat.FinishNow();                \
        name##Stat.PrintTime(#name, msgLevel); \
    }

} // namespace Vim2Ds
