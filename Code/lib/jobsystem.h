#ifndef JOB_SYSTEM_JOBSYSTEM_H
#define JOB_SYSTEM_JOBSYSTEM_H

#include <vector>
#include <mutex>
#include <deque>
#include <fstream>
#include <unordered_map>

constexpr int JOB_TYPE_ANY = -1;

class JobWorkerThread;

enum JobStatus
{
    JOB_STATUS_NEVER_SEEN,
    JOB_STATUS_QUEUED,
    JOB_STATUS_RUNNING,
    JOB_STATUS_COMPLETED,
    JOB_STATUS_RETIRED,
    NUM_JOB_STATUSES
};

struct JobHistoryEntry
{
    JobHistoryEntry(int jobType, JobStatus jobStatus)
        : m_jobType(jobType), m_jobStatus(jobStatus) {}

    int m_jobType = -1;
    int m_jobStatus = JOB_STATUS_NEVER_SEEN;
};

class Job;

class JobSystem
{
    friend class JobWorkerThread;

public:
    JobSystem();
    ~JobSystem();

    void Stop();
    void Resume();

    static JobSystem *CreateOrGet();
    static void Destroy();
    int totalJobs = 0;

    void CreateWorkerThread(const char *uniqueName, unsigned long workerJobChannels = 0xFFFFFFFF);
    void DestroyWorkerThread(const char *uniqueName);
    void QueueJob(Job *job);

    // Status queries
    JobStatus GetJobStatus(int jobID) const;
    bool IsJobComplete(int jobID) const;
    bool areJobsRunning()
    {
        bool temp;
        m_jobsRunningMutex.lock();
        temp = m_jobsRunning.size() != 0;
        m_jobsRunningMutex.unlock();
        return temp;
    }
    bool areJobsCompleted()
    {
        bool temp;
        m_jobsCompletedMutex.lock();
        temp = m_jobsCompleted.size() != 0;
        m_jobsCompletedMutex.unlock();
        return temp;
    }

    std::string FinishJob(int jobID);
    std::string FinishCompletedJobs();

    void Register(std::string name, Job *fnptr);
    int CreateJob(std::string jobType, std::string input);
    std::vector<std::string> GetJobTypes();
    void DestroyJob(int jobID);

private:
    Job *ClaimAJob(unsigned long channels);
    void OnJobCompleted(Job *jobJustExecuted);

    static JobSystem *s_jobSystem;

    std::vector<JobWorkerThread *> m_workerThreads;
    mutable std::mutex m_workerThreadsMutex;
    std::deque<Job *> m_jobsQueued;
    std::deque<Job *> m_jobsRunning;
    std::deque<Job *> m_jobsCompleted;
    mutable std::mutex m_jobsQueuedMutex;
    mutable std::mutex m_jobsRunningMutex;
    mutable std::mutex m_jobsCompletedMutex;

    std::vector<JobHistoryEntry> m_jobHistory;
    mutable int m_jobHistoryLowestActiveIndex = 0;
    mutable std::mutex m_jobHistoryMutex;

    std::unordered_map<std::string, Job *> jobs;
};

#endif // JOB_SYSTEM_JOBSYSTEM_H