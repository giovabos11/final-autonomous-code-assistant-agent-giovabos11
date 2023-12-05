#include "interpreter.h"

using namespace std;

// Compile Job.
// string command: A Makefile command.
// Returns a JSON object containing the compile output and the project name.
string compile(string command)
{
    json temp;
    string output;
    int returnCode;
    array<char, 128> buffer;

    // Get project name
    string projectName = command.substr(command.find_last_of(' ') + 1, command.length());

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
        return "popen Failed: Failed to open pipe";
    }

    // Read until the end of the process
    while (fgets(buffer.data(), 128, pipe) != NULL)
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

    // Store output and project name in a json object
    temp["output"] = output;
    temp["file_name"] = projectName + ".json";

    return temp.dump();
}

// Function to generate JSON from a single JSON compilation error
void generateJson(string &str, json &outputJson)
{
    // If no errors or the string is empty, exit
    if (str.size() == 0 || str == "[]")
        return;

    // Parse file
    json data;
    try
    {
        data = json::parse(str);
    }
    catch (const exception &e)
    {
        std::cout << "Error parsing the console output: Invalid json format" << std::endl;
        return;
    }

    string sourceFilename, kind, message;
    int lineNumber, columnNumber;

    // Go through every error/warning
    for (int i = 0; i < data.size(); i++)
    {
        // Get either error or warning
        kind = data[i]["kind"];
        // Ignore everithing that is not a warning or error
        if (kind != "error" && kind != "warning" && kind != "note")
            continue;
        // std::cout << data[i]["kind"] << std::endl;

        // Get message
        message = data[i]["message"];
        // std::cout << message << std::endl;

        // Get main error/warning source line number (first caret) in the file
        lineNumber = data[i]["locations"][0]["caret"]["line"];
        // std::cout << lineNumber << std::endl;

        // Get main error/warning source line number (first caret) in the file
        columnNumber = data[i]["locations"][0]["caret"]["display-column"];
        // std::cout << columnNumber << std::endl;

        // Get file name
        sourceFilename = data[i]["locations"][0]["caret"]["file"];
        // std::cout << sourceFilename << std::endl;

        // Get 2 lines above and bellow if possible (code snippet)
        ifstream sourceFile(sourceFilename);
        int currentLineNumber = 1;
        string currrentLine = "", snippet = ""; // MAKE ONE LINER
        int codeIndex = 0;
        while (!sourceFile.eof() && !sourceFile.fail())
        {
            getline(sourceFile, currrentLine);
            if (currentLineNumber == lineNumber - 2 ||
                currentLineNumber == lineNumber - 1 ||
                currentLineNumber == lineNumber ||
                currentLineNumber == lineNumber + 1 ||
                currentLineNumber == lineNumber + 2)
                snippet += currrentLine + "\n";

            currentLineNumber++;
        }
        sourceFile.close();

        // Build json object
        outputJson[sourceFilename] += {{"file", sourceFilename}, {kind, message}, {"line", lineNumber}, {"column", columnNumber}, {"code", snippet}};
    }
}

// Parse JSON output Job.
// string output: A JSON object that contains the whole output of compilation, which may have one or
//                more errors in JSON format, and the project name.
// Returns a JSON that contains the error for each file formatted in JSON and the project name.
string parseFile(string output)
{
    json outputJson = json::parse(output);
    json tempJson;
    tempJson["errors"] = {};
    // Split the different errors in separete json objects to parse
    int startIndex = 0, endIndex = 0;
    string temp;
    for (int i = 0; i <= outputJson["output"].size(); i++)
    {
        if (outputJson["output"][i] == '\n' || i == outputJson["output"].size())
        {
            endIndex = i;
            temp = "";
            temp.append(outputJson["output"], startIndex, endIndex - startIndex);
            generateJson(temp, tempJson["errors"]);
            startIndex = endIndex + 1;
        }
    }

    tempJson["file_name"] = outputJson["file_name"];

    return tempJson.dump();
}

// Output errors to a file.
// string output: A JSON object that contains the formatted error for each file, and the project name.
// Returns a string that indicates that the job is done.
string outputToFile(string output)
{
    json outputJson = json::parse(output);
    // Write to file
    ofstream o("../Data/output_" + outputJson["file_name"].dump());
    o << setw(4) << outputJson["errors"] << endl;
    o.close();
    return "Done!";
}

// Job that calls LLM.
// string a: A JSON object that contains an ip adress, a prompt, a model name and an optional api key.
// Returns a string, which is either the model response or an error message.
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
    js.RegisterJob("compile", new Job(compile, 2));
    js.RegisterJob("parse_file", new Job(parseFile, 3));
    js.RegisterJob("output_to_file", new Job(outputToFile, 4));

    // Import prompt and error files
    string prompt1, error1, error2, error3;
    prompt1 = openFile("../Data/gpt4all-mistral-prompt.txt");
    error1 = openFile("../Data/error1.json");
    error2 = openFile("../Data/error2.json");
    error3 = openFile("../Data/error3.json");

    // Spin off jobs
    // string job1 = js.CreateJob("{\"job_type\": \"call_LLM\", \"input\": {\"ip\": \"http://localhost:4891/v1/chat/completions\", \"prompt\": \"" + prompt1 + error1 + "\", \"model\": \"mistral-7b-instruct-v0.1.Q4_0\"}}");
    // string job2 = js.CreateJob("{\"job_type\": \"call_LLM\", \"input\": {\"ip\": \"http://localhost:4891/v1/chat/completions\", \"prompt\": \"" + prompt1 + error2 + "\", \"model\": \"mistral-7b-instruct-v0.1.Q4_0\"}}");
    // string job3 = js.CreateJob("{\"job_type\": \"call_LLM\", \"input\": {\"ip\": \"http://localhost:4891/v1/chat/completions\", \"prompt\": \"" + prompt1 + error3 + "\", \"model\": \"mistral-7b-instruct-v0.1.Q4_0\"}}");
    string job1 = js.CreateJob("{\"job_type\": \"compile\", \"input\": \"MinGW32-make project1\"}");

    // Get job IDs
    int job1ID = json::parse(job1)["id"];
    // int job2ID = json::parse(job2)["id"];
    // int job3ID = json::parse(job3)["id"];

    // Get Job statuses
    cout << "Job ID " << job1ID << " status: " << json::parse(js.JobStatus(job1))["status"] << endl
         // cout << "Job ID " << job2ID << " status: " << json::parse(js.JobStatus(job2))["status"] << endl;
         // cout << "Job ID " << job3ID << " status: " << json::parse(js.JobStatus(job3))["status"] << endl
         << endl;

    // Check job status and try to complete the jobs
    string output1, output2, output3;

    while (json::parse(js.AreJobsRunning())["are_jobs_running"])
    {
        //  Wait to complete all the jobs
    }

    // Get job outputs
    output1 = json::parse(js.CompleteJob(job1))["output"];
    // output2 = json::parse(js.CompleteJob(job2))["output"];
    // output3 = json::parse(js.CompleteJob(job3))["output"];

    // Print job outputs
    cout << "Job ID " << job1ID << " output: " << output1 << endl
         << endl;
    // cout << "Job ID " << job2ID << " output: " << output2 << endl
    //      << endl;
    // cout << "Job ID " << job3ID << " output: " << output3 << endl
    //      << endl;

    // Get Job statuses
    cout << "Job ID " << job1ID << " status: " << json::parse(js.JobStatus(job1))["status"] << endl;
    // cout << "Job ID " << job2ID << " status: " << json::parse(js.JobStatus(job2))["status"] << endl;
    // cout << "Job ID " << job3ID << " status: " << json::parse(js.JobStatus(job3))["status"] << endl;

    // Destroy Job System
    js.DestroyJobSystem();

    return 0;
}