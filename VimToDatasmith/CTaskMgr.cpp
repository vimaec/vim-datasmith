/// Copyright (c) 2021 VIM
// Licensed under the MIT License 1.0

#include "CTaskMgr.h"

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

CTaskMgr& CTaskMgr::Get() {
    if (STaskMgr == nullptr) {
        STaskMgr = new CTaskMgr();
    }
    return *STaskMgr;
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
: mThreadingEnabled(true) {
    if (mThreadingEnabled) {
        // One thread by processor
        mNbProcessors = std::thread::hardware_concurrency();
        if (mNbProcessors == 0) {
            mNbProcessors = 1;
        }
        mTreads.resize(mNbProcessors);
        mNbRunning = mNbProcessors;
        for (unsigned i = 0; i < mNbProcessors; i++) {
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
    if (mThreadingEnabled) {
        // Add the task to the queue
        {
            std::unique_lock<std::mutex> lk(mAccessControl);
            if (mTerminate)
                throw std::runtime_error("Adding task to a terminated CTaskMgr");
            mTaskQueue.push(inTask);
        }
        mThreadControlConditionVariable.notify_all();
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
    clock_t previous = clock();
    while (mNbRunning != 0 || (!mTaskQueue.empty() && !mTerminate)) {
        clock_t current = clock();
        if (current - previous > CLOCKS_PER_SEC) {
            previous = current;
            TraceF(".");
        }
        lk.unlock();
        mThreadControlConditionVariable.notify_one();
        std::this_thread::sleep_for(1ms);
        lk.lock();
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

// Join all task before continuing
void CTaskMgr::CTaskJointer::Join(bool inRethrow) {
    if (mTaskCount != 0) {
        CTaskMgr& mgr = CTaskMgr::Get();
        std::unique_lock<std::mutex> lk(mgr.mAccessControl);
        while (mTaskCount != 0) {
            lk.unlock();
            mgr.mThreadControlConditionVariable.notify_one();
            std::this_thread::sleep_for(1ms);
            lk.lock();
        }
    }
    if (inRethrow && mGotExceptionCount != 0) {
        uint32_t gotExceptionCount = mGotExceptionCount;
        mGotExceptionCount = 0;
        ThrowMessage("CTaskJointer::Join - Rethrow the task exception (%d)", uint32_t(gotExceptionCount));
    }
}

} // namespace Vim2Ds
