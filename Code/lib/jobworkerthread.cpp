#include "jobworkerthread.h"
#include "jobsystem.h"

JobWorkerThread::JobWorkerThread(const char *uniqueName, unsigned long workerJobChannels, JobSystem *jobSystem) : m_uniqueName(uniqueName),
                                                                                                                  m_workerJobChannels(workerJobChannels),
                                                                                                                  m_jobSystem(jobSystem)
{
}

JobWorkerThread::~JobWorkerThread()
{
    // If we haven't already signal thread then we should exit as soon as it can (after its current job if any)
    ShutDown();

    // Block
    m_thread->join();
    delete m_thread;
    m_thread = nullptr;
}

void JobWorkerThread::StartUp()
{
    m_thread = new std::thread(WorkerThreadMain, this);
}

void JobWorkerThread::Work()
{
    while (!IsStopping())
    {
        m_workerStatusMutex.lock();
        unsigned long workerJobChannels = m_workerJobChannels;
        m_workerStatusMutex.unlock();

        Job *job = m_jobSystem->ClaimAJob(m_workerJobChannels);
        if (job)
        {
            job->Execute(job->input);
            m_jobSystem->OnJobCompleted(job);
        }

        // How fast you want to pull jobs
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
}

void JobWorkerThread::ShutDown()
{
    m_workerStatusMutex.lock();
    m_isStopping = true;
    m_workerStatusMutex.unlock();
}

void JobWorkerThread::TurnOn()
{
    m_workerStatusMutex.lock();
    m_isStopping = false;
    m_workerStatusMutex.unlock();
}

bool JobWorkerThread::IsStopping() const
{
    m_workerStatusMutex.lock();
    // Create a local variable to return a variable that cannot be modified by other threads
    bool shouldClose = m_isStopping;
    m_workerStatusMutex.unlock();

    return shouldClose;
}

void JobWorkerThread::SetWorkerJobChannels(unsigned long workerJobChannels)
{
    m_workerStatusMutex.lock();
    m_workerJobChannels = workerJobChannels;
    m_workerStatusMutex.unlock();
}

void JobWorkerThread::WorkerThreadMain(void *workThreadObject)
{
    // Void pointers don't know the size of memory to grab.
    // When cast, now it knows how many memory to grab.
    JobWorkerThread *thisWorker = (JobWorkerThread *)workThreadObject; // Take void pointer and cast it as a JobWorkerThread.
    thisWorker->Work();
}