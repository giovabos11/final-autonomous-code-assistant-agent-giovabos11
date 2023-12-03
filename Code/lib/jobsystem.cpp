#include <iostream>
#include <cstring>
#include "jobsystem.h"
#include "jobworkerthread.h"

JobSystem *JobSystem::s_jobSystem = nullptr;

typedef void (*JobCallback)(Job *completedJob);

JobSystem::JobSystem()
{
    m_jobHistory.reserve(256 * 1024);
}

JobSystem::~JobSystem()
{
    m_workerThreadsMutex.lock();
    int numWorkerThreads = (int)m_workerThreads.size();

    // First, tell each worker thread to stop picking up jobs
    for (int i = 0; i < numWorkerThreads; i++)
    {
        m_workerThreads[i]->ShutDown();
    }

    while (!m_workerThreads.empty())
    {
        delete m_workerThreads.back();
        m_workerThreads.pop_back();
    }
    m_workerThreadsMutex.unlock();
}

void JobSystem::Stop()
{
    m_workerThreadsMutex.lock();
    int numWorkerThreads = (int)m_workerThreads.size();

    // Tell each worker thread to stop picking up jobs
    for (int i = 0; i < numWorkerThreads; i++)
    {
        m_workerThreads[i]->ShutDown();
    }
    m_workerThreadsMutex.unlock();
}

void JobSystem::Resume()
{
    m_workerThreadsMutex.lock();
    int numWorkerThreads = (int)m_workerThreads.size();

    // Tell each worker thread to pick up jobs again
    for (int i = 0; i < numWorkerThreads; i++)
    {
        m_workerThreads[i]->TurnOn();
    }
    m_workerThreadsMutex.unlock();
}

JobSystem *JobSystem::CreateOrGet()
{
    if (!s_jobSystem)
    {
        s_jobSystem = new JobSystem();
    }
    return s_jobSystem;
}

void JobSystem::Destroy()
{
    if (s_jobSystem)
    {
        delete s_jobSystem;
        s_jobSystem = nullptr;
    }
}

void JobSystem::CreateWorkerThread(const char *uniqueName, unsigned long workerJobChannels)
{
    JobWorkerThread *newWorker = new JobWorkerThread(uniqueName, workerJobChannels, this);
    m_workerThreadsMutex.lock();
    m_workerThreads.push_back(newWorker);
    m_workerThreads.back()->StartUp();
    m_workerThreadsMutex.unlock();
}

void JobSystem::DestroyWorkerThread(const char *uniqueName)
{
    m_workerThreadsMutex.lock();
    JobWorkerThread *doomedWorker = nullptr;
    std::vector<JobWorkerThread *>::iterator it = m_workerThreads.begin();

    for (; it != m_workerThreads.end(); ++it)
    {
        if (std::strcmp((*it)->m_uniqueName, uniqueName) == 0)
        {
            doomedWorker = *it;
            m_workerThreads.erase(it);
            break;
        }
    }
    m_workerThreadsMutex.unlock();

    if (doomedWorker)
    {
        doomedWorker->ShutDown();
        delete doomedWorker;
    }
}

void JobSystem::QueueJob(Job *job)
{
    m_jobsQueuedMutex.lock();

    m_jobHistoryMutex.lock();
    m_jobHistory.emplace_back(JobHistoryEntry(job->m_jobType, JOB_STATUS_QUEUED));
    m_jobHistoryMutex.unlock();

    m_jobsQueued.push_back(job);
    m_jobsQueuedMutex.unlock();
}

JobStatus JobSystem::GetJobStatus(int jobID) const
{
    m_jobHistoryMutex.lock();

    JobStatus jobStatus = JOB_STATUS_NEVER_SEEN;
    if (jobID < m_jobHistory.size())
    {
        jobStatus = (JobStatus)m_jobHistory[jobID].m_jobStatus;
    }

    m_jobHistoryMutex.unlock();

    return jobStatus;
}

bool JobSystem::IsJobComplete(int jobID) const
{
    return (GetJobStatus(jobID)) == (JOB_STATUS_COMPLETED);
}

std::string JobSystem::FinishCompletedJobs()
{
    std::deque<Job *> jobsCompleted;
    std::string output = "null";

    m_jobsCompletedMutex.lock();
    jobsCompleted.swap(m_jobsCompleted);
    m_jobsCompletedMutex.unlock();

    for (Job *job : jobsCompleted)
    {
        output = job->JobCompleteCallback();
        m_jobHistoryMutex.lock();
        m_jobHistory[job->m_jobID].m_jobStatus = JOB_STATUS_RETIRED;
        m_jobHistoryMutex.unlock();
        delete job;
    }
    return output;
}

std::string JobSystem::FinishJob(int jobID)
{
    // std::cout << "Here" << std::endl;
    // m_jobsCompletedMutex.lock();
    // std::cout << "Completed queue size: " << m_jobsCompleted.size() << std::endl;
    // Job *thisCompletedJob = nullptr;
    // for (auto jcIter = m_jobsCompleted.begin(); jcIter != m_jobsCompleted.end(); ++jcIter)
    // {
    //     Job *someCompletedJob = *jcIter;
    //     std::cout << someCompletedJob->m_jobID << ", " << GetJobStatus(someCompletedJob->m_jobID) << std::endl;
    // }
    // m_jobsCompletedMutex.unlock();

    std::string output = "null";
    while (!IsJobComplete(jobID))
    {
        // Wait for job to complete first
        std::this_thread::yield();
    }

    JobStatus jobStatus = GetJobStatus(jobID);
    if (jobStatus == JOB_STATUS_NEVER_SEEN || jobStatus == JOB_STATUS_RETIRED)
    {
        std::cout << "ERROR: Waiting for Job (#" << jobID << ") - no such job in JobSystem." << std::endl;
    }

    m_jobsCompletedMutex.lock();
    Job *thisCompletedJob = nullptr;
    for (auto jcIter = m_jobsCompleted.begin(); jcIter != m_jobsCompleted.end(); ++jcIter)
    {
        Job *someCompletedJob = *jcIter;
        if (someCompletedJob->m_jobID == jobID)
        {
            thisCompletedJob = someCompletedJob;
            m_jobsCompleted.erase(jcIter);
            break;
        }
    }
    m_jobsCompletedMutex.unlock();

    if (thisCompletedJob == nullptr)
    {
        std::cout << "ERROR: Job #" << jobID << " was status complete but not found in completed list." << std::endl;
    }
    output = thisCompletedJob->JobCompleteCallback();

    m_jobHistoryMutex.lock();
    m_jobHistory[thisCompletedJob->m_jobID].m_jobStatus = JOB_STATUS_RETIRED;
    m_jobHistoryMutex.unlock();

    delete thisCompletedJob;

    return output;
}

void JobSystem::OnJobCompleted(Job *jobJustExecuted)
{
    totalJobs++;
    m_jobsCompletedMutex.lock();
    m_jobsRunningMutex.lock();

    std::deque<Job *>::iterator runningJobItr = m_jobsRunning.begin();
    for (; runningJobItr != m_jobsRunning.end(); ++runningJobItr)
    {
        if (jobJustExecuted == *runningJobItr)
        {
            m_jobHistoryMutex.lock();
            m_jobsRunning.erase(runningJobItr);
            m_jobsCompleted.push_back(jobJustExecuted);
            m_jobHistory[jobJustExecuted->m_jobID].m_jobStatus = JOB_STATUS_COMPLETED;
            m_jobHistoryMutex.unlock();
            break;
        }
    }
    m_jobsRunningMutex.unlock();
    m_jobsCompletedMutex.unlock();
}

Job *JobSystem::ClaimAJob(unsigned long channels)
{
    m_jobsQueuedMutex.lock();
    m_jobsRunningMutex.lock();

    Job *claimedJob = nullptr;
    std::deque<Job *>::iterator queuedJobIter = m_jobsQueued.begin();
    for (; queuedJobIter != m_jobsQueued.end(); ++queuedJobIter)
    {
        Job *queuedJob = *queuedJobIter;

        if ((queuedJob->m_jobChannels & channels) != 0)
        {
            claimedJob = queuedJob;

            m_jobHistoryMutex.lock();
            m_jobsQueued.erase(queuedJobIter);
            m_jobsRunning.push_back(claimedJob);
            m_jobHistory[claimedJob->m_jobID].m_jobStatus = JOB_STATUS_RUNNING;
            m_jobHistoryMutex.unlock();
            break;
        }
    }

    m_jobsRunningMutex.unlock();
    m_jobsQueuedMutex.unlock();

    return claimedJob;
}

void JobSystem::Register(std::string name, Job *fnptr)
{
    // Create a new key for the function pointer
    jobs[name] = fnptr;
}

int JobSystem::CreateJob(std::string jobType, std::string input)
{
    // Clone the job from the function pointer and queue it
    Job *newJob = jobs[jobType];
    Job *cloned = new Job(*newJob);
    cloned->input = input;
    QueueJob(cloned);
    return cloned->GetUniqueID();
}

std::vector<std::string> JobSystem::GetJobTypes()
{
    std::vector<std::string> keys;
    for (auto key : jobs)
    {
        keys.push_back(key.first);
    }
    return keys;
}

void JobSystem::DestroyJob(int jobID)
{
    // Clear the job from any queue
    m_jobsQueuedMutex.lock();
    Job *thisJob1 = nullptr;
    for (auto jcIter = m_jobsQueued.begin(); jcIter != m_jobsQueued.end(); ++jcIter)
    {
        Job *someJob = *jcIter;
        if (someJob->m_jobID == jobID)
        {
            thisJob1 = someJob;
            m_jobsQueued.erase(jcIter);
            break;
        }
    }
    m_jobsQueuedMutex.unlock();

    m_jobsRunningMutex.lock();
    Job *thisJob2 = nullptr;
    for (auto jcIter = m_jobsRunning.begin(); jcIter != m_jobsRunning.end(); ++jcIter)
    {
        Job *someJob = *jcIter;
        if (someJob->m_jobID == jobID)
        {
            thisJob2 = someJob;
            // Finish the job before erasing it form the queue
            FinishJob(thisJob2->GetUniqueID());
            m_jobsRunning.erase(jcIter);
            break;
        }
    }
    m_jobsRunningMutex.unlock();

    m_jobsCompletedMutex.lock();
    Job *thisJob3 = nullptr;
    for (auto jcIter = m_jobsCompleted.begin(); jcIter != m_jobsCompleted.end(); ++jcIter)
    {
        Job *someJob = *jcIter;
        if (someJob->m_jobID == jobID)
        {
            thisJob3 = someJob;
            m_jobsCompleted.erase(jcIter);
            break;
        }
    }
    m_jobsCompletedMutex.unlock();
}