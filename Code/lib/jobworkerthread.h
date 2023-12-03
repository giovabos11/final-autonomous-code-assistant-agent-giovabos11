#ifndef JOB_SYSTEM_JOBWORKERTHREAD_H
#define JOB_SYSTEM_JOBWORKERTHREAD_H

#include <mutex>
#include <map>
#include <deque>
#include <vector>
#include <thread>
#include "job.h"

class JobSystem;

class JobWorkerThread
{
    friend class JobSystem;

private:
    JobWorkerThread(const char *uniqueName, unsigned long workerJobChannels, JobSystem *jobSystem);
    ~JobWorkerThread();

    void StartUp();  // Kick off the actual thread, which will call Work()
    void Work();     // Called in the private thread, blocks forever until StopWorking() is called
    void ShutDown(); // Signal that work should stop at next opportunity
    void TurnOn();

    bool IsStopping() const;
    void SetWorkerJobChannels(unsigned long workerJobChannels);
    static void WorkerThreadMain(void *workThreadObject);

    const char *m_uniqueName;
    unsigned long m_workerJobChannels = 0xFFFFFFFF;
    bool m_isStopping = false;
    JobSystem *m_jobSystem = nullptr;
    std::thread *m_thread = nullptr;
    mutable std::mutex m_workerStatusMutex;
};

#endif // JOB_SYSTEM_JOBWORKERTHREAD_H