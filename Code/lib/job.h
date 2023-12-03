#ifndef JOB_SYSTEM_JOB_H
#define JOB_SYSTEM_JOB_H

#include <mutex>
#include <map>
#include <deque>
#include <vector>
#include <thread>
#include <string>

typedef std::string (*fnptr)(std::string);
static int s_nextJobID = 0;

class Job
{
    friend class JobSystem;
    friend class JobWorkerThread;

public:
    Job(fnptr ptr, int jobType = -1, unsigned long jobChannels = 0xFFFFFFFF) : ptr(ptr), m_jobChannels(jobChannels), m_jobType(jobType)
    {
        m_jobID = s_nextJobID++;
    }

    Job(Job &other)
    {
        m_jobID = s_nextJobID++;

        this->ptr = other.ptr;
        this->m_jobID = m_jobID;
        this->m_jobType = other.m_jobType;
        this->m_jobChannels = other.m_jobChannels;
    }

    ~Job() {}

    void Execute(std::string input) { output = ptr(input); }
    std::string JobCompleteCallback() { return output; };
    int GetUniqueID() const { return m_jobID; }

    std::string input;

private:
    fnptr ptr = NULL;
    std::string output;
    int m_jobID = -1;
    int m_jobType = -1;
    unsigned long m_jobChannels = 0xFFFFFFFF;
};

#endif