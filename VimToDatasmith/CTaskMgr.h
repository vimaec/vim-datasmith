// Copyright (c) 2021 VIM
// Licensed under the MIT License 1.0

#pragma once

#include "VimToDatasmith.h"

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace Vim2Ds {

class CTaskMgr {
  public:
    // Task class, your class must inherit from this class.
    class ITask {
      public:
        virtual void Run() = 0;

      protected:
        virtual ~ITask(){};
    };

    // Constructor
    CTaskMgr();

    // Destructor
    ~CTaskMgr();

    // Add task (if inRunNow, the task is run immediatly in current thread)
    void AddTask(ITask* InTask);

    // Wait until all task have been processed
    void Join();

    static CTaskMgr* GetMgr();

    static void DeleteMgr();

  private:
    static void RunITask(CTaskMgr* inMgr);

    // Get the next task to be run
    ITask* GetTask();

    // Threads used (≈ nomber of cpu)
    std::vector<std::unique_ptr<std::thread>> mTreads;

    // Fifo tasks queue
    std::queue<ITask*> mTaskQueue;

    // Control access on this object (for queue operations)
    std::mutex mAccessControl;

    // Control queue management
    std::condition_variable mThreadControlConditionVariable;

    // Number of thread currently running
    volatile unsigned mNbRunning = 0;

    // Destructor have been called, dont execute task anymore
    volatile bool mTerminate = false;

    // False: tasks to run in main threads, true: Task run in thread
    bool mTreadingEnabled;
};

} // namespace Vim2Ds
