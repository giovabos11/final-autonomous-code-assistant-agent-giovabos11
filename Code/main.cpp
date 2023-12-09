#include "interpreter.h"

using namespace std;

// Function to clean scaped JSON
void cleanJson(string &str)
{
    // Remove double quotes surrounding the string
    str = str.substr(1, str.size() - 2);
    // Replace \" to "
    size_t start_pos = 0;
    string from = "\\\"", to = "\"";
    while ((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
    // Replace \\" to \"
    start_pos = 0;
    from = "\\\\\"", to = "\\\"";
    while ((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}

// Compile Job.
// string command: A Makefile command.
// Returns a JSON object containing the compile output and the project name.
string compile(string command)
{
    cleanJson(command);
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
    temp["file_name"] = "output_" + projectName + ".json";

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
        cout << "Error parsing the console output: Invalid json format" << std::endl;
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
    cleanJson(output);
    json outputJson = json::parse(output);
    json tempJson;
    tempJson["content"] = {};

    // Clean errors JSON
    string errors = outputJson["output"].dump();
    cleanJson(errors);
    size_t start_pos = 0;
    string from = "\\\\n", to = "\n";
    while ((start_pos = errors.find(from, start_pos)) != std::string::npos)
    {
        errors.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }

    // Split the different errors in separete json objects to parse
    int startIndex = 0, endIndex = 0;
    string temp;
    for (int i = 0; i <= errors.size(); i++)
    {
        if (errors[i] == '\n' || i == errors.size())
        {
            endIndex = i;
            temp = "";
            temp.append(errors, startIndex, endIndex - startIndex);
            generateJson(temp, tempJson["content"]);
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
    try
    {
        outputJson["content"];
    }
    catch (const exception &e)
    {
        cleanJson(output);
        try
        {
            outputJson = json::parse(output);
        }
        catch (const exception &e)
        {
            return "Error generating the output file: Invalid json format";
        }
    }
    //  Write to file
    ofstream o("../Data/" + outputJson["file_name"].dump().substr(1, outputJson["file_name"].dump().size() - 2));

    // Clean content before printing to file
    string content = outputJson["content"].dump();
    string from, to;
    if (outputJson["content"].is_string())
    {
        cleanJson(content);
        from = "\\n";
        to = "\n";
    }
    else
    {
        from = "\\\\n";
        to = "\\n";
    }
    size_t start_pos = 0;
    while ((start_pos = content.find(from, start_pos)) != std::string::npos)
    {
        content.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
    // Print content to file
    o << content << endl;

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

    // Create interpreter object
    Interpreter interpreter;

    js.CreateJobSystem();
    js.CreateThreads();

    // Register all jobs
    js.RegisterJob("call_LLM", new Job(callLLM, 1));
    js.RegisterJob("output_to_file", new Job(outputToFile, 2));

    // Ask the user to input a project
    cout << "Enter the name of the make command: ";
    string projectName = "";
    cin >> projectName;
    cout << endl;

    /// ---------------------- LLM FLOWSCRIPT GENERATION ---------------------- ///

    // Import prompt and error files
    string promptFlowscript;
    promptFlowscript = openFile("../Data/flowscript-prompt.txt");

    // Loop LLM Flowscript generation until it generates valid FlowScript
    do
    {
        // Spin off job and get job ID
        string jobFlowscript = js.CreateJob("{\"job_type\": \"call_LLM\", \"input\": {\"ip\": \"http://localhost:4891/v1/chat/completions\", \"prompt\": \"" + promptFlowscript + "\", \"model\": \"mistral-7b-instruct-v0.1.Q4_0\"}}");
        int jobFlowscriptID = json::parse(jobFlowscript)["id"];

        cout << "Generate FlowScript Job running... ";

        // Check job status and try to complete the jobs
        while (json::parse(js.AreJobsRunning())["are_jobs_running"])
        {
            //  Wait to complete all the jobs
        }

        // // Get job outputs
        string outputFlowscript;
        outputFlowscript = json::parse(js.CompleteJob(jobFlowscript))["output"];

        // Check if there were any errors with the call
        if (outputFlowscript == "Invalid arguments provided" ||
            outputFlowscript == "LLM connection error" ||
            outputFlowscript == "Output JSON error" ||
            outputFlowscript == "Failed to open pipe")
        {
            cout << "LLM call failed" << endl
                 << endl;
            return 0;
        }

        // Clean LLM output
        outputFlowscript = outputFlowscript.substr(1, outputFlowscript.size() - 2);
        size_t start_pos = 0;
        string from = "`", to = "";
        while ((start_pos = outputFlowscript.find(from, start_pos)) != std::string::npos)
        {
            outputFlowscript.replace(start_pos, from.length(), to);
            start_pos += to.length();
        }
        // Print job output
        cout << "FlowScript Generated: " << endl
             << outputFlowscript << endl
             << endl;

        /// ---------------------- LLM FLOWSCRIPT TO FILE ---------------------- ///

        /// TEST: manually set outputFlowscript to the right FlowScript file to test the rest of the code
        outputFlowscript = "digraph jobs {\\n    input -> compile -> parse_file -> output_to_file -> output\\n}\\n";

        // Spin off job and get job ID
        string jobFlowscriptFile = js.CreateJob("{\"job_type\": \"output_to_file\", \"input\": {\"file_name\" : \"compiling_pipeline.dot\", \"content\": \"" + outputFlowscript + "\"}}");
        int jobFlowscriptFileID = json::parse(jobFlowscriptFile)["id"];

        cout << "FlowScript to File Job running... ";

        // Check job status and try to complete the jobs
        while (json::parse(js.AreJobsRunning())["are_jobs_running"])
        {
            //  Wait to complete all the jobs
        }

        // Get job outputs
        string outputFlowscriptFile;
        outputFlowscriptFile = json::parse(js.CompleteJob(jobFlowscriptFile))["output"];

        // Print job output
        cout << outputFlowscriptFile << endl;

        /// ---------------------- RUN FLOWSCRIPT TO GET ERRORS ---------------------- ///

        // Load flowscript file
        interpreter.loadFile("../Data/compiling_pipeline.dot");

        // Register all jobs for interpreter
        interpreter.registerJob("compile", new Job(compile, 3));
        interpreter.registerJob("parse_file", new Job(parseFile, 4));
        interpreter.registerJob("output_to_file", new Job(outputToFile, 5));

        // Pass the input
#ifdef __linux__
        interpreter.setInput("make " + projectName);
#elif _WIN32
        interpreter.setInput("MinGW32-make " + projectName);
#else
        interpreter.setInput("make " + projectName);
#endif

        // Parse flowscript file
        interpreter.parse();

        // If there is an error with the FlowScript, print error details
        if (interpreter.getErrorCode() != 0)
        {
            cout << "Couldn't compile flowscript: " << endl;
            cout << interpreter.getErrorMessage() << endl;
            cout << "Error line: " << interpreter.getErrorLine() << endl
                 << endl;

            cout << "Generating FlowScript again" << endl
                 << endl;
            continue;
        }
        else
        {
            // Run FlowScript to compile, parse file, and ouput the errors
            cout << "Running FlowScript to get errors... " << interpreter.run() << endl
                 << endl;
        }
    } while (interpreter.getErrorCode() == 1);

    /// ---------------------- TEST CODE AND ITERATE UNTIL IT FIXES IT ---------------------- ///

    json errorJson;
    int first = true;
    do
    {
        cout << "Checking if source code has compilation errors... ";
        // First, check if project does not have errors
        ifstream errorFile("../Data/output_" + projectName + ".json");
        string error = "", line = "";
        if (errorFile.is_open())
        {
            while (getline(errorFile, line))
                error += line + " ";
            errorFile.close();
        }
        errorJson = json::parse(error);

        // If no errors, finish the program
        if (errorJson.is_null())
        {
            cout << "All errors fixed!" << endl;
            return 0;
        }
        else
        {
            cout << "Errors detected!" << endl;
        }

        /// ---------------------- LLM TRIES TO GENERATE A FIX FOR THE CODE ---------------------- ///

        cout << "Fix C++ Code Job running... ";
        // Read prompt to pass to the LLM
        error = openFile("../Data/output_" + projectName + ".json");
        string prompt;
        if (first)
            prompt = openFile("../Data/fix-code-prompt.txt");
        else
            prompt = openFile("../Data/fix-code-prompt.txt");

        // Read api key
        ifstream dotEnv("./.env");
        string apiKey = "";
        line = "";
        if (dotEnv.is_open())
        {
            while (getline(dotEnv, line))
                apiKey += line + " ";
            dotEnv.close();
        }
        else
        {
            cout << "You need an API key to call the LLM" << endl;
            return 0;
        }
        if (apiKey == "")
        {
            cout << "You need an API key to call the LLM" << endl;
            return 0;
        }

        // Call LLM to fix the code
        string jobFixCode = js.CreateJob("{\"job_type\": \"call_LLM\", \"input\": {\"ip\": \"https://api.openai.com/v1/chat/completions\", \"prompt\": \"" + prompt + error + "\", \"model\": \"gpt-3.5-turbo\", \"key\": \"" + apiKey + "\"}}");
        int jobFixCodeID = json::parse(jobFixCode)["id"];

        // Check job status and try to complete the job
        while (json::parse(js.AreJobsRunning())["are_jobs_running"])
        {
            //  Wait to complete all the jobs
        }

        // Get job output
        string outputFixCode = json::parse(js.CompleteJob(jobFixCode))["output"];

        // Check if there were any errors with the call
        if (outputFixCode == "Invalid arguments provided" ||
            outputFixCode == "LLM connection error" ||
            outputFixCode == "Output JSON error" ||
            outputFixCode == "Failed to open pipe")
        {
            cout << "LLM call failed" << endl
                 << endl;
            return 0;
        }

        // Clean LLM Output
        cleanJson(outputFixCode);
        // Replace \\n to \n
        size_t start_pos = 0;
        string from = "\\\\n", to = "\\n";
        while ((start_pos = outputFixCode.find(from, start_pos)) != std::string::npos)
        {
            outputFixCode.replace(start_pos, from.length(), to);
            start_pos += to.length();
        }

        // Store LLM output in a JSON object
        json fixJson;
        try
        {
            fixJson = json::parse(outputFixCode);
        }
        // If it fails to read the JSON, ask the LLM to generate it again
        catch (const exception &e)
        {
            cout << "LLM returned poorly formated JSON" << endl
                 << endl;
            continue;
        }

        cout << "ChatGPT Fix provided: " << endl;

        // For each fix
        for (int i = 0; i < fixJson.size(); i++)
        {
            // Fix code in the given line
            int lineNumber = fixJson[i]["line"];
            string fix = fixJson[i]["fixed_code"], file_name = fixJson[i]["file_name"];

            // Print fixes and resolutions
            cout << endl
                 << "File Name: " << file_name << endl;
            cout << "Line number: " << lineNumber << endl;
            cout << "Fixed code: " << endl
                 << fix << endl
                 << endl;

            ifstream inputFile(file_name);
            vector<string> lines;
            string line = "";
            if (inputFile.is_open())
            {
                while (getline(inputFile, line))
                {
                    lines.push_back(line);
                }
                inputFile.close();
            }
            int currentLine = -2; // Number of lines to grab above and bellow * -1
            while (lineNumber + currentLine - 1 < 0)
                currentLine++;
            lines[lineNumber + currentLine - 1] = "";
            for (int i = 0; i < fix.size(); i++)
            {
                if (fix[i] == '\n')
                {
                    currentLine++;
                    if (lineNumber + currentLine - 1 < lines.size() && lineNumber + currentLine - 1 >= 0 && i != fix.size() - 1)
                        lines[lineNumber + currentLine - 1] = "";
                }
                else
                {
                    lines[lineNumber + currentLine - 1] += fix[i];
                }
            }
            ofstream o(file_name);
            for (string line : lines)
                o << line << endl;
            o.close();

            if (i < fixJson.size() - 1)
                cout << "=================================" << endl;
        }

        // Run FlowScript again to compile, parse file, and ouput the errors
        cout << "Running FlowScript to get errors... " << interpreter.run() << endl
             << endl;

        // Change variable to pass try-again prompt
        first = false;
    } while (!errorJson.is_null());

    // Destroy Job System
    js.DestroyJobSystem();

    return 0;
}