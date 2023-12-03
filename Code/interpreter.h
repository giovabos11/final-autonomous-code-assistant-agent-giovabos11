#pragma once
#include <fstream>

#include "nodes.h"

class Interpreter
{
private:
    JobSystemInterface js;

    // DECLARE FLOWSCRIPT RESERVED KEYWORDS
    std::string keywords[4] = {"digraph",
                               "input",
                               "output",
                               "split"};
    enum var_def
    {
        INPUT,
        OUTPUT,
        JOB,
        IF_STATEMENT,
        SWITCH,
        MULTI,
        SIZE
    };
    struct connection
    {
        std::string from = "";
        std::string to = "";
        std::string shape = "";
        std::string style = "";
        std::string label = "";
    };
    std::unordered_map<std::string, std::pair<var_def, std::string>> jobs;
    std::vector<connection> connections;

    std::vector<std::string> codeLines;

    std::string input;
    std::string output;
    std::vector<std::string> processes;

    int errorCode;
    std::string errorMessage;
    int errorLine;

    void generateError(int errorCode, std::string errorMessage, int errorLine);

    bool isReserved(std::string token);

    std::vector<connection> findFrom(std::string find);

public:
    Interpreter()
    {
        js.CreateJobSystem();
        js.CreateThreads();
        errorCode = 0;
        errorMessage = "";
        errorLine = -1;
        input = "";
        output = "";
    }

    int getErrorCode() { return errorCode; }
    std::string getErrorMessage() { return errorMessage; }
    int getErrorLine() { return errorLine; }

    void setInput(std::string input) { this->input = input; }
    std::string getOutput() { return this->output; }

    void loadFile(std::string filename);
    void registerJob(std::string name, Job *ptr);
    void parse();
    std::string run();
};