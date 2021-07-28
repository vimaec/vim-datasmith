// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "DebugTools.h"
#include "VimToDsWarningsDisabler.h"

namespace Vim2Ds {

// Get cpu and real time
class FTimeStat {
  public:
    // Contructor (Get current process CPU time and real time)
    FTimeStat() { ReStart(); }

    // Copy Contructor
    FTimeStat(const FTimeStat& InOther) {
        CpuTime = InOther.CpuTime;
        RealTime = InOther.RealTime;
    }

    // Reset to current process CPU time and real time
    void ReStart();

    // Cumulate time from the other
    void AddDiff(const FTimeStat& InOther);

    // Print time differences
    void PrintDiff(const char* InStatLabel, const FTimeStat& InStart, EP2DB inMsgLevel = kP2DB_Trace);

    // Return the ReStart cpu time
    double GetCpuTime() const { return CpuTime; }

    // Return the ReStart real time
    double GetRealTime() const { return RealTime; }

    // Tool get current real time clock
    static double RealTimeClock();

    // Tool get process CPU real time clock
    static double CpuTimeClock();

  private:
    // Time at the creation / ReStart of this object
    double CpuTime;
    double RealTime;
};

// Macro to measure code execution time
#define MeasureTime(name, code, msgLevel)                  \
    {                                                      \
        FTimeStat name##start;                             \
        code;                                              \
        FTimeStat name##end;                               \
        name##end.PrintDiff(#name, name##start, msgLevel); \
    }

} // namespace Vim2Ds
