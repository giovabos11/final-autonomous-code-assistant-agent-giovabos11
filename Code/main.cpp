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
        output = json::parse(output)["choices"][0]["text"].dump();
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
            output = "JSON Error";
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
    prompt2 = openFile("../Data/gpt4all-falcon-prompt.txt");
    error1 = openFile("../Data/error1.json");
    error2 = openFile("../Data/error2.json");
    error3 = openFile("../Data/error3.json");

    // Spin off jobs
    string job1 = js.CreateJob("{\"job_type\": \"call_LLM\", \"input\": {\"ip\": \"http://localhost:4891/v1\", \"prompt\": \"" + prompt1 + error1 + "\", \"model\": \"mistral-7b-instruct-v0.1.Q4_0\"}}");
    string job2 = js.CreateJob("{\"job_type\": \"call_LLM\", \"input\": {\"ip\": \"http://localhost:4891/v1\", \"prompt\": \"" + prompt1 + error2 + "\", \"model\": \"mistral-7b-instruct-v0.1.Q4_0\"}}");
    string job3 = js.CreateJob("{\"job_type\": \"call_LLM\", \"input\": {\"ip\": \"http://localhost:4891/v1\", \"prompt\": \"" + prompt1 + error3 + "\", \"model\": \"mistral-7b-instruct-v0.1.Q4_0\"}}");
    string job4 = js.CreateJob("{\"job_type\": \"call_LLM\", \"input\": {\"ip\": \"http://localhost:4891/v1\", \"prompt\": \"" + prompt2 + error1 + "\", \"model\": \"gpt4all-falcon-q4_0\"}}");
    string job5 = js.CreateJob("{\"job_type\": \"call_LLM\", \"input\": {\"ip\": \"http://localhost:4891/v1\", \"prompt\": \"" + prompt2 + error2 + "\", \"model\": \"gpt4all-falcon-q4_0\"}}");
    string job6 = js.CreateJob("{\"job_type\": \"call_LLM\", \"input\": {\"ip\": \"http://localhost:4891/v1\", \"prompt\": \"" + prompt2 + error3 + "\", \"model\": \"gpt4all-falcon-q4_0\"}}");

    // Get job IDs
    int job1ID = json::parse(job1)["id"];
    int job2ID = json::parse(job2)["id"];
    int job3ID = json::parse(job3)["id"];
    int job4ID = json::parse(job4)["id"];
    int job5ID = json::parse(job5)["id"];
    int job6ID = json::parse(job6)["id"];

    // Get Job statuses
    cout << "Job ID " << job1ID << " status: " << json::parse(js.JobStatus(job1))["status"] << endl;
    cout << "Job ID " << job2ID << " status: " << json::parse(js.JobStatus(job2))["status"] << endl;
    cout << "Job ID " << job3ID << " status: " << json::parse(js.JobStatus(job3))["status"] << endl;
    cout << "Job ID " << job4ID << " status: " << json::parse(js.JobStatus(job4))["status"] << endl;
    cout << "Job ID " << job5ID << " status: " << json::parse(js.JobStatus(job5))["status"] << endl;
    cout << "Job ID " << job6ID << " status: " << json::parse(js.JobStatus(job6))["status"] << endl
         << endl;

    // Check job status and try to complete the jobs
    string output1, output2, output3, output4, output5, output6;

    while (json::parse(js.AreJobsRunning())["are_jobs_running"])
    {
        //  Wait to complete all the jobs
    }

    // Get job outputs
    output1 = json::parse(js.CompleteJob(job1))["output"];
    output2 = json::parse(js.CompleteJob(job2))["output"];
    output3 = json::parse(js.CompleteJob(job3))["output"];
    output4 = json::parse(js.CompleteJob(job4))["output"];
    output5 = json::parse(js.CompleteJob(job5))["output"];
    output6 = json::parse(js.CompleteJob(job6))["output"];

    // Print job outputs
    cout << "Job ID " << job1ID << " output: " << output1 << endl
         << endl;
    cout << "Job ID " << job2ID << " output: " << output2 << endl
         << endl;
    cout << "Job ID " << job3ID << " output: " << output3 << endl
         << endl;
    cout << "Job ID " << job4ID << " output: " << output4 << endl
         << endl;
    cout << "Job ID " << job5ID << " output: " << output5 << endl
         << endl;
    cout << "Job ID " << job6ID << " output: " << output6 << endl
         << endl;

    // Get Job statuses
    cout << "Job ID " << job1ID << " status: " << json::parse(js.JobStatus(job1))["status"] << endl;
    cout << "Job ID " << job2ID << " status: " << json::parse(js.JobStatus(job2))["status"] << endl;
    cout << "Job ID " << job3ID << " status: " << json::parse(js.JobStatus(job3))["status"] << endl;
    cout << "Job ID " << job4ID << " status: " << json::parse(js.JobStatus(job4))["status"] << endl;
    cout << "Job ID " << job5ID << " status: " << json::parse(js.JobStatus(job5))["status"] << endl;
    cout << "Job ID " << job6ID << " status: " << json::parse(js.JobStatus(job6))["status"] << endl;

    // Destroy Job System
    js.DestroyJobSystem();

    return 0;
}