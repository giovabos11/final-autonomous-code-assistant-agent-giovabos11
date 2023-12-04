#include "interpreter.h"

using namespace std;

// Job that calls LLM
string callLLM(string a)
{
    json input = json::parse(a);
    string output;
    int returnCode;
    array<char, 2048> buffer;

    // Build command to execute script
    string command;
    if (!input.contains("key"))
    {
        command = "python callLLM.py --ip " + input["ip"].dump();
        command += " --prompt " + input["prompt"].dump();
        command += " --model " + input["model"].dump();
    }
    else
    {
        command = "python callLLM.py --ip " + input["ip"].dump();
        command += " --prompt " + input["prompt"].dump();
        command += " --model " + input["model"].dump();
        command += " -k " + input["key"].dump();
    }

    // Redirect cerr to cout
    command.append(" 2>&1");

    // Open pile and run command
#ifdef __linux__
    FILE *pipe = popen(command.c_str(), "r");
#elif _WIN32
    FILE *pipe = _popen(command.c_str(), "r");
#else
    FILE *pipe = popen(command.c_str(), "r");
#endif

    if (!pipe)
    {
        return "Failed to open pipe";
    }

    // Read until the end of the process
    while (fgets(buffer.data(), 2048, pipe) != NULL)
    {
        output.append(buffer.data());
    }

    // Close pipe and get the return code
#ifdef __linux__
    returnCode = pclose(pipe);
#elif _WIN32
    returnCode = _pclose(pipe);
#else
    returnCode = pclose(pipe);
#endif

    try
    {
        output = json::parse(output)["choices"][0]["message"]["content"].dump();
    }
    catch (const json::parse_error &e)
    {
        if (output.find("usage: callLLM.py [-h] -i IP -p PROMPT -m MODEL [-k KEY]") != string::npos)
        {
            output = "Invalid arguments provided";
        }
        else if (output.find("Connection error") != string::npos)
        {
            output = "LLM connection error";
        }
        else
        {
            output = "Output JSON error";
        }
    }

    return output;
}

// Function to open file and return content
string openFile(string path)
{
    string output = "", line = "";
    ifstream inputFile;
    inputFile.open(path);
    if (inputFile.is_open())
    {
        while (getline(inputFile, line))
        {
            // Scape characters that can break the JSON object
            size_t start_pos = 0;
            string from = "\\", to = "\\\\";
            while ((start_pos = line.find(from, start_pos)) != std::string::npos)
            {
                line.replace(start_pos, from.length(), to);
                start_pos += to.length();
            }
            start_pos = 0;
            from = "\"", to = "\\\"\\\"";
            while ((start_pos = line.find(from, start_pos)) != std::string::npos)
            {
                line.replace(start_pos, from.length(), to);
                start_pos += to.length();
            }
            output += line + " ";
        }
        inputFile.close();
    }
    return output;
}

int main()
{
    // Create job system object
    JobSystemInterface js;

    js.CreateJobSystem();
    js.CreateThreads();

    // Register all jobs
    js.RegisterJob("call_LLM", new Job(callLLM, 1));

    // Import prompt and error files
    string prompt1, prompt2, error1, error2, error3;
    prompt1 = openFile("../Data/gpt4all-mistral-prompt.txt");
    error1 = openFile("../Data/error1.json");
    error2 = openFile("../Data/error2.json");
    error3 = openFile("../Data/error3.json");

    // Spin off jobs
    string job1 = js.CreateJob("{\"job_type\": \"call_LLM\", \"input\": {\"ip\": \"http://localhost:4891/v1/chat/completions\", \"prompt\": \"" + prompt1 + error1 + "\", \"model\": \"mistral-7b-instruct-v0.1.Q4_0\"}}");
    string job2 = js.CreateJob("{\"job_type\": \"call_LLM\", \"input\": {\"ip\": \"http://localhost:4891/v1/chat/completions\", \"prompt\": \"" + prompt1 + error2 + "\", \"model\": \"mistral-7b-instruct-v0.1.Q4_0\"}}");
    string job3 = js.CreateJob("{\"job_type\": \"call_LLM\", \"input\": {\"ip\": \"http://localhost:4891/v1/chat/completions\", \"prompt\": \"" + prompt1 + error3 + "\", \"model\": \"mistral-7b-instruct-v0.1.Q4_0\"}}");

    // Get job IDs
    int job1ID = json::parse(job1)["id"];
    int job2ID = json::parse(job2)["id"];
    int job3ID = json::parse(job3)["id"];

    // Get Job statuses
    cout << "Job ID " << job1ID << " status: " << json::parse(js.JobStatus(job1))["status"] << endl;
    cout << "Job ID " << job2ID << " status: " << json::parse(js.JobStatus(job2))["status"] << endl;
    cout << "Job ID " << job3ID << " status: " << json::parse(js.JobStatus(job3))["status"] << endl;

    // Check job status and try to complete the jobs
    string output1, output2, output3;

    while (json::parse(js.AreJobsRunning())["are_jobs_running"])
    {
        //  Wait to complete all the jobs
    }

    // Get job outputs
    output1 = json::parse(js.CompleteJob(job1))["output"];
    output2 = json::parse(js.CompleteJob(job2))["output"];
    output3 = json::parse(js.CompleteJob(job3))["output"];

    // Print job outputs
    cout << "Job ID " << job1ID << " output: " << output1 << endl
         << endl;
    cout << "Job ID " << job2ID << " output: " << output2 << endl
         << endl;
    cout << "Job ID " << job3ID << " output: " << output3 << endl
         << endl;

    // Get Job statuses
    cout << "Job ID " << job1ID << " status: " << json::parse(js.JobStatus(job1))["status"] << endl;
    cout << "Job ID " << job2ID << " status: " << json::parse(js.JobStatus(job2))["status"] << endl;
    cout << "Job ID " << job3ID << " status: " << json::parse(js.JobStatus(job3))["status"] << endl;

    // Destroy Job System
    js.DestroyJobSystem();

    return 0;
}