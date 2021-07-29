// Copyright (c) 2021 VIM
// Licensed under the MIT License 1.0

#pragma once

#include "DebugTools.h"
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

    // Lightweight task synchronization mechanism
    class CTaskJointer;

    // Lightweight joinable task
    class CJoinableTask : public CTaskMgr::ITask {
      public:
        // Destructor
        virtual ~CJoinableTask();

        // Start the task
        void Start(CTaskJointer* inTaskJointer);

        virtual void Run() override;

        virtual void RunTask() = 0;

      private:
        CTaskJointer* mTaskJointer = nullptr;
    };

    // Lightweight joinable functor task
    template <class Parameter> class TJoinableFunctorTask;

    // Constructor
    CTaskMgr();

    // Destructor
    ~CTaskMgr();

    unsigned GetNbProcessors() const { return mNbProcessors; }

    // Add task (if inRunNow, the task is run immediatly in current thread)
    void AddTask(ITask* InTask);

    template <class Task> void AddTasks(std::vector<Task>& inTasks) {
        for (Task& task : inTasks)
            AddTask(&task);
    }

    // Wait until all task have been processed
    void Join();

    static CTaskMgr& Get();

    static void DeleteMgr();

    // Control access on this object (for queue operations)
    std::mutex mAccessControl;

    // Control queue management
    std::condition_variable mThreadControlConditionVariable;

  private:
    static void RunITask(CTaskMgr* inMgr);

    // Get the next task to be run
    ITask* GetTask();

    // Threads used (≈ nomber of cpu)
    std::vector<std::unique_ptr<std::thread>> mTreads;

    // Fifo tasks queue
    std::queue<ITask*> mTaskQueue;

    unsigned mNbProcessors = 1;

    // Number of thread currently running
    volatile unsigned mNbRunning = 0;

    // Destructor have been called, dont execute task anymore
    volatile bool mTerminate = false;

    // False: tasks to run in main threads, true: Task run in thread
    bool mTreadingEnabled;
};

// Lightweight task synchronization mechanism
class CTaskMgr::CTaskJointer {
  public:
    // Constructor
    CTaskJointer(const char* inName)
    : mName(inName)
    , mTaskCount(0)
    , mGotExceptionCount(0) {}

    // Destructor, insure all tasks are jointed
    ~CTaskJointer() {
        Join(false);
        if (mGotExceptionCount != 0)
            DebugF("CTaskJointer::~CTaskJointer - %d exceptions hasn't been catched [Jointer=%s]\n", uint32_t(mGotExceptionCount), mName);
    }

    // A task is added
    void AddTask(CJoinableTask* inTask) {
        ++mTaskCount;
        CTaskMgr::Get().AddTask(inTask);
    }

    // Join all task before continuing
    void Join(bool inRethrow = true);

    // A task finish it work
    void RemoveTask(bool inGotException) {
        TestAssert(mTaskCount != 0);
        if (--mTaskCount == 0)
            CTaskMgr::Get().mThreadControlConditionVariable.notify_one();
        if (inGotException)
            ++mGotExceptionCount;
    }

    // How many tasks finish because of an exception?
    uint32_t TakeException() {
        uint32_t gotExceptionCount = mGotExceptionCount;
        mGotExceptionCount = 0;
        return gotExceptionCount;
    }

    // The joint name (for debug purpose)
    const char* GetName() const { return mName; }

  protected:
    // The joint name (for debug purpose)
    const char* const mName;

    // Number of task to be executed
    volatile std::atomic<uint32_t> mTaskCount;

    // Number of task that finish with an exception
    std::atomic<uint32_t> mGotExceptionCount;
};

// Destructor
inline CTaskMgr::CJoinableTask::~CJoinableTask() {
    if (mTaskJointer != nullptr)
        FatalF("CJoinableTask::~CJoinableTask - Deleting a running task [Jointer=%s]\n", mTaskJointer->GetName());
}

// Start the task
inline void CTaskMgr::CJoinableTask::Start(CTaskMgr::CTaskJointer* inTaskJointer) {
    TestAssert(mTaskJointer == nullptr && inTaskJointer != nullptr);
    mTaskJointer = inTaskJointer;
    mTaskJointer->AddTask(this);
}

// Execute the task
inline void CTaskMgr::CJoinableTask::Run() {
    TestPtr(mTaskJointer);
    try {
        RunTask();
        if (mTaskJointer != nullptr) {
            mTaskJointer->RemoveTask(false);
            mTaskJointer = nullptr;
        }
        delete this;
    } catch (...) {
        if (mTaskJointer != nullptr) {
            mTaskJointer->RemoveTask(true);
            mTaskJointer = nullptr;
        }
        delete this;
        throw;
    }
}

// Lightweight joinable functor task (permit to call a lambda with parameter on a thread)
template <class Parameter> class CTaskMgr::TJoinableFunctorTask : public CTaskMgr::CJoinableTask {
  public:
    // Function type
    typedef void (*const Functor)(Parameter inParameter);

    // Constructor
    TJoinableFunctorTask(Functor inFunctor, const Parameter& inParameter)
    : mFunctor(inFunctor)
    , mParameter(inParameter) {
        TestAssert(mFunctor);
    }

    // Execute the functor
    virtual void RunTask() override { mFunctor(mParameter); }

  protected:
    // The functor
    Functor mFunctor;

    // The functor parameters
    Parameter mParameter;
};

} // namespace Vim2Ds
