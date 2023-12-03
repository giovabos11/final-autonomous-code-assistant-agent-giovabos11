#pragma once
#include <iostream>
#include <string>

#include "jobsystem.h"
#include "job.h"

#include "json.hpp"
using json = nlohmann::json;

class JobSystemInterface
{

public:
    void CreateJobSystem();
    void StopJobSystem();
    void ResumeJobSystem();
    void DestroyJobSystem();

    void CreateThreads();

    std::string CreateJob(std::string input);
    void DestroyJob(std::string input);
    std::string JobStatus(std::string id);
    std::string CompleteJob(std::string input);
    std::string GetJobTypes();
    std::string AreJobsRunning();

    void RegisterJob(std::string name, Job *ptr);

private:
    JobSystem *js;
};