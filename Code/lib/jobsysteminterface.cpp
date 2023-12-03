#include "jobsysteminterface.h"

void JobSystemInterface::CreateJobSystem()
{
    // Create job system object
    js = JobSystem::CreateOrGet();
}

void JobSystemInterface::StopJobSystem()
{
    // Stop Job System
    js->Stop();
}

void JobSystemInterface::ResumeJobSystem()
{
    // Resume Job System
    js->Resume();
}

void JobSystemInterface::DestroyJobSystem()
{
    // Destroy Job System
    js->Destroy();
}

void JobSystemInterface::CreateThreads()
{
    // Create the maximum thread amount supported by the system
    for (int i = 0; i < std::thread::hardware_concurrency() - 1; i++)
    {
        js->CreateWorkerThread(("Thread" + std::to_string(i)).c_str(), 0xFFFFFFFF);
    }
}

std::string JobSystemInterface::CreateJob(std::string input)
{
    json temp = json::parse(input);
    int jobID = js->CreateJob(temp["job_type"], temp["input"].dump());
    temp["id"] = jobID;
    return temp.dump();
}

void JobSystemInterface::DestroyJob(std::string input)
{
    // Destroy Job
    js->DestroyJob(json::parse(input)["id"]);
}

std::string JobSystemInterface::JobStatus(std::string input)
{
    // Return the job status
    json temp = json::parse(input);
    temp["status"] = (int)js->GetJobStatus(json::parse(input)["id"]);
    return temp.dump();
}

std::string JobSystemInterface::CompleteJob(std::string input)
{
    json temp = json::parse(input);
    temp["output"] = js->FinishJob(temp["id"]);
    // Finish job
    return temp.dump();
}

std::string JobSystemInterface::AreJobsRunning()
{
    json temp;
    temp["are_jobs_running"] = js->areJobsRunning();
    // Return if jobs are running or completed
    return temp.dump();
}

void JobSystemInterface::RegisterJob(std::string name, Job *ptr)
{
    // Register job
    js->Register(name, ptr);
}

std::string JobSystemInterface::GetJobTypes()
{
    json temp;
    std::vector<std::string> jobTypes = js->GetJobTypes();
    for (int i = 0; i < jobTypes.size(); i++)
    {
        temp += jobTypes[i];
    }
    return temp.dump();
}