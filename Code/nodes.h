#pragma once
#include <string>
#include <vector>
#include <unordered_map>

#include "./lib/jobsysteminterface.h"

class Node
{
public:
    virtual std::string execute(std::string input) { return ""; }
};

static std::unordered_map<std::string, Node *> nodes;

class JobNode : public Node
{
public:
    std::string name;
    std::string next_ptr;
    JobSystemInterface *js;

    JobNode(JobSystemInterface *js, std::string name, std::string next_ptr) : js(js), name(name), next_ptr(next_ptr) {}
    std::string execute(std::string input)
    {
        json temp;
        temp["job_type"] = name;
        temp["input"] = input;

        // Spin off the job and wait to complete
        std::string job = js->CreateJob(temp.dump());
        while (json::parse(js->AreJobsRunning())["are_jobs_running"])
        {
        }
        std::string result = json::parse(js->CompleteJob(job))["output"];

        if (next_ptr != "")
        {
            return nodes[next_ptr]->execute(result);
        }
        return result;
    }
};

class IfNode : public Node
{
public:
    std::string condition;
    std::string true_ptr;
    std::string false_ptr;

    IfNode(std::string condition, std::string true_ptr, std::string false_ptr) : condition(condition), true_ptr(true_ptr), false_ptr(false_ptr) {}
    std::string execute(std::string input)
    {
        return "";
    }
};

class SwitchNode : public Node
{
public:
    std::string condition;
    // Value, ptr
    std::vector<std::pair<std::string, std::string>> ptrs;

    SwitchNode(std::vector<std::pair<std::string, std::string>> ptrs, std::string condition) : ptrs(ptrs), condition(condition) {}
    std::string execute(std::string input)
    {
        return "";
    }
};

class MultiNode : public Node
{
public:
    std::vector<std::string> jobs;
    std::string next_ptr;

    MultiNode(std::vector<std::string> jobs, std::string next_node) : jobs(jobs), next_ptr(next_ptr) {}
    std::string execute(std::string input)
    {
        return "";
    }
};

class InputNode : public Node
{
public:
    std::string next_ptr;

    InputNode(std::string next_ptr) : next_ptr(next_ptr) {}
    std::string execute(std::string input)
    {
        if (next_ptr != "")
        {
            return nodes[next_ptr]->execute(input);
        }
        return input;
    }
};

class OutputNode : public Node
{
public:
    std::string execute(std::string input)
    {
        return input;
    }
};