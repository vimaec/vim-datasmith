/// Copyright (c) 2021 VIM
// Licensed under the MIT License 1.0

#include "CTaskMgr.h"
#include "DebugTools.h"

#include <chrono>
#include <stdexcept>
using namespace std::literals;

namespace Vim2Ds {

static CTaskMgr* STaskMgr = nullptr;

void CTaskMgr::RunITask(CTaskMgr* inMgr) {
    AutoReleasePool {
#if macOS
        pthread_setname_np("CTaskMgr::RunITask");
#endif
        ITask* myATask = inMgr->GetTask();
        while (myATask) {
            try {
                myATask->Run();
            } catch (std::exception& e) {
                DebugF("CTaskMgr::RunITask - Catch std exception %s\n", e.what());
            } catch (...) {
                DebugF("CTaskMgr::RunITask - Catch unknown exception\n");
            }
            myATask = inMgr->GetTask();
        }
    }
}

CTaskMgr* CTaskMgr::GetMgr() {
    if (STaskMgr == nullptr) {
        STaskMgr = new CTaskMgr();
    }
    return STaskMgr;
}

void CTaskMgr::DeleteMgr() {
    if (STaskMgr) {
        CTaskMgr* Tmp = STaskMgr;
        STaskMgr = nullptr;
        delete Tmp;
    }
}

// Constructor
CTaskMgr::CTaskMgr()
: mTreadingEnabled(true) {
    if (mTreadingEnabled) {
        // One thread by processor
        unsigned nbProcessors = std::thread::hardware_concurrency();
        if (nbProcessors == 0) {
            nbProcessors = 1;
        }
        mTreads.resize(nbProcessors);
        mNbRunning = nbProcessors;
        for (unsigned i = 0; i < nbProcessors; i++) {
            mTreads[i].reset(new std::thread(RunITask, this));
        }
    }
}

// Destructor
CTaskMgr::~CTaskMgr() {
    {
        std::lock_guard<std::mutex> lock(mAccessControl);
        mTerminate = true;
    }
    Join();

    mThreadControlConditionVariable.notify_all();
    size_t nbThreads = mTreads.size();
    for (size_t i = 0; i < nbThreads; i++) {
        mTreads[i]->join();
    }

    // Empty the task
    while (!mTaskQueue.empty())
        mTaskQueue.pop();
}

// Add task
void CTaskMgr::AddTask(ITask* inTask) {
    if (mTreadingEnabled) {
        // Add the task to the queue
        {
            std::unique_lock<std::mutex> lk(mAccessControl);
            if (mTerminate)
                throw std::runtime_error("Adding task to a terminated CTaskMgr");
            mTaskQueue.push(inTask);
        }
        mThreadControlConditionVariable.notify_one();
    } else {
        try {
            inTask->Run();
        } catch (std::exception& e) {
            DebugF("CTaskMgr::AddTask - Catch std exception %s\n", e.what());
        } catch (...) {
            DebugF("CTaskMgr::AddTask - Catch unknown exception\n");
        }
    }
}

// Wait until all task have been processed
void CTaskMgr::Join() {
    bool hasWorks = false;
    std::unique_lock<std::mutex> lk(mAccessControl);
    if (mNbRunning != 0 || !mTaskQueue.empty()) {
        TraceF("CTaskMgr::Join - Wait for %ld task to be processed ", mTaskQueue.size());
        hasWorks = true;
    }
    while (mNbRunning != 0 || (!mTaskQueue.empty() && !mTerminate)) {
        TraceF(".");
        //        TraceF("CTaskMgr::Join - Wait for %ld task to be processed\n", mTaskQueue.size());
        mThreadControlConditionVariable.notify_all(); // Wake all waiting threads
        mThreadControlConditionVariable.wait_for(lk, 100ms, [this] { return mNbRunning == 0 && (mTaskQueue.empty() || mTerminate); });
    }
    if (hasWorks)
        TraceF("\nCTaskMgr::Join - Done\n");
}

// Get the next task to be run
CTaskMgr::ITask* CTaskMgr::GetTask() {
    ITask* task = nullptr;

    // Try to get the front task
    std::unique_lock<std::mutex> lk(mAccessControl);
    --mNbRunning;
    if (!mTerminate) {
        if (!mTaskQueue.empty()) {
            task = mTaskQueue.front();
            mTaskQueue.pop();
        }

        if (task == nullptr) {
            // No task, wait for added one
            mThreadControlConditionVariable.wait(lk, [this, &task] {
                if (!mTaskQueue.empty()) {
                    task = mTaskQueue.front();
                    mTaskQueue.pop();
                }
                return task != nullptr || mTerminate;
            });
        }

        if (task != nullptr)
            ++mNbRunning;
    }

    return task;
}

} // namespace Vim2Ds
