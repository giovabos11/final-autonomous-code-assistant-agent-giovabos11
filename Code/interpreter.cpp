#include "interpreter.h"

void Interpreter::loadFile(std::string filename)
{
    std::ifstream myfile(filename);
    std::string line;

    if (myfile.is_open())
    {
        while (getline(myfile, line))
        {
            // Delete double spaces
            std::size_t doubleSpace = line.find("  ");
            while (doubleSpace != std::string::npos)
            {
                line.erase(doubleSpace, 1);
                doubleSpace = line.find("  ");
            }
            // Append line to code lines
            codeLines.push_back(line);
        }
        myfile.close();
    }
    else
    {
        std::cout << "Unable to open file" << std::endl;
    }
}

void Interpreter::registerJob(std::string name, Job *ptr)
{
    js.RegisterJob(name, ptr);
}

void Interpreter::generateError(int errorCode, std::string errorMessage, int errorLine)
{
    std::cout << "ERROR!" << std::endl;
    this->errorCode = errorCode;
    this->errorMessage = errorMessage;
    this->errorLine = errorLine;
    return;
}

bool Interpreter::isReserved(std::string token)
{
    for (int i = 0; i < 4; i++)
    {
        if (keywords[i] == token)
        {
            return true;
        }
    }
    return false;
}

std::vector<Interpreter::connection> Interpreter::findFrom(std::string find)
{
    std::vector<connection> temp;
    for (auto c : connections)
    {
        if (c.from == find)
        {
            temp.push_back(c);
        }
    }
    return temp;
}

void Interpreter::parse()
{
    int lineNumber;
    // For every line of code
    for (int i = 0; i < codeLines.size(); i++)
    {
        // std::cout << codeLines[i] << ": ";
        // Increment the line number
        lineNumber = i + 1;
        // Split each character
        std::string leftToken = "";
        std::string rightToken = "";
        bool left = true;
        std::string label = "";

        // Create a temporary connection object
        connection temp;
        // bool isJob = true;
        bool hasArrow = false;
        bool argumentEnds = false;

        // For each character
        for (int j = 0; j < codeLines[i].size(); j++)
        {
            char c = codeLines[i][j];
            // std::cout << c << ": ";
            // Syntax Error
            if (argumentEnds)
            {
                generateError(1, "Statements after brakets are not valid", lineNumber);
                return;
            }
            if (std::isdigit(c) && leftToken.size() == 0)
            {
                generateError(1, "Process cannot start with digit", lineNumber);
                return;
            }

            // Process name
            else if (std::isalnum(c))
                if (left)
                    leftToken += c;
                else
                    rightToken += c;

            // Argument list
            else if (c == '[')
            {
                // Syntax errors
                if (codeLines[i].find("]") == std::string::npos)
                {
                    generateError(1, "Missing closing bracket", lineNumber);
                    return;
                }
                if (codeLines[i].find("]") < j)
                {
                    generateError(1, "Closing bracket before opening bracket", lineNumber);
                    return;
                }

                // Get argument
                std::string arg = codeLines[i].substr(j + 1, codeLines[i].find("]") - codeLines[i].find("[") - 1);
                // std::cout << arg << std::endl;

                // Handle label
                if (arg.find("label=\"") != std::string::npos)
                {
                    // Get label
                    for (int x = arg.find("label=\"") + 7; x < arg.size(); x++)
                    {
                        if (arg[x] != '\"')
                        {
                            label += arg[x];
                        }
                        else
                        {
                            break;
                        }
                    }
                    // std::cout << label << std::endl;
                    //  Add label to connection
                    temp.label = label;
                }

                // Handle process
                if (arg.find("shape=\"square\"") != std::string::npos)
                {
                    // std::cout << "square" << std::endl;
                    // std::cout << leftToken << ": JOB, " << "" << std::endl;
                    //  Add to list of jobs
                    jobs[leftToken] = std::make_pair(JOB, label);
                    // isJob = false;
                    //  Add shape to connection
                    temp.shape = "square";
                }
                // Handle conditional (if statement)
                else if (arg.find("shape=\"diamond\"") != std::string::npos)
                {
                    // Error
                    if (label == "")
                    {
                        generateError(1, "If statement requires a condition", lineNumber);
                        return;
                    }
                    if (hasArrow)
                    {
                        generateError(1, "If declaration cannot be declared with a dependency", lineNumber);
                        return;
                    }
                    // std::cout << "diamond" << std::endl;
                    // std::cout << leftToken << ": IF_STATEMENT, " << label << std::endl;
                    // Add to list of jobs
                    jobs[leftToken] = std::make_pair(IF_STATEMENT, label);
                    // isJob = false;
                    //  Add shape to connection
                    temp.shape = "diamond";
                }
                // Handle conditional (switch statement)
                else if (arg.find("shape=\"trapezium\"") != std::string::npos)
                {
                    // std::cout << "trapezium" << std::endl;
                    // Error
                    if (label == "")
                    {
                        generateError(1, "Switch statement requires a condition", lineNumber);
                        return;
                    }
                    // std::cout << leftToken << ": SWITCH, " << label << std::endl;
                    // Add to list of jobs
                    jobs[leftToken] = std::make_pair(SWITCH, label);
                    // isJob = false;
                    //  Add shape to connection
                    temp.shape = "trapezium";
                }
                // Handle branching (multi-process)
                else if (arg.find("shape=\"point\"") != std::string::npos)
                {
                    // std::cout << "point" << std::endl;
                    if (leftToken != "split")
                    {
                        generateError(1, "Point name needs to be split, not \"" + leftToken + "\"", lineNumber);
                        return;
                    }
                    // std::cout << leftToken << ": MULTI, " << "" << std::endl;
                    // Add to list of jobs
                    jobs[leftToken] = std::make_pair(MULTI, label);
                    // isJob = false;
                    //  Add shape to connection
                    temp.shape = "point";
                }
                // Handle branching process (multi-process)
                else if (arg.find("style=\"dashed\"") != std::string::npos)
                {
                    // std::cout << "multi" << std::endl;
                    // Error
                    if (rightToken != "" && leftToken != "split")
                    {
                        if (jobs.find(leftToken) == jobs.end())
                        {
                            generateError(1, "Dashed property needs to come from a split or another dashed process", lineNumber);
                            return;
                        }
                        if (jobs[leftToken].first != MULTI)
                        {
                            generateError(1, "Dashed property needs to come from a split or another dashed process", lineNumber);
                            return;
                        }
                    }
                    // std::cout << rightToken << ": MULTI, " << "" << std::endl;
                    // Add to list of jobs
                    jobs[rightToken] = std::make_pair(MULTI, label);
                    // isJob = false;
                    //  Add shape to connection
                    temp.style = "dashed";
                }

                // Skip to the end of the argument
                j += arg.size() + 1;
                // STATEMENT SHOULD END AFTER THIS (EVERYTHING ELSE IS IGNORED)
                argumentEnds = true;
                break;
            }

            // Dependency (Arrow)
            else if (c == '-' && codeLines[i][j + 1] == '>')
            {
                // std::cout << "arrow ";
                hasArrow = true;
                // STATE DEPENDENCY HERE
                if (rightToken != "")
                {
                    // Error
                    if (rightToken == "input")
                    {
                        generateError(1, "Input block cannot have a dependency", lineNumber);
                        return;
                    }
                    if (leftToken == "output" && rightToken != "")
                    {
                        generateError(1, "Output block cannot be a dependency", lineNumber);
                        return;
                    }
                    // std::cout << rightToken << " depends on " << leftToken << std::endl;
                    // Add to list of jobs if not already there
                    if (jobs.find(leftToken) == jobs.end())
                        if (leftToken == "input")
                            jobs[leftToken] = std::make_pair(INPUT, label);
                        else if (leftToken == "output")
                            jobs[leftToken] = std::make_pair(OUTPUT, label);
                        else
                            jobs[leftToken] = std::make_pair(JOB, label);
                    if (jobs.find(rightToken) == jobs.end())
                        if (rightToken == "output")
                            jobs[rightToken] = std::make_pair(OUTPUT, label);
                        else
                            jobs[rightToken] = std::make_pair(JOB, label);

                    temp.from = leftToken;
                    temp.to = rightToken;
                    connections.push_back(temp);

                    leftToken = rightToken;
                    rightToken = "";
                }
                else
                {
                    // Change side
                    left = !left;
                    // Add to list of jobs if not already there
                    if (jobs.find(leftToken) == jobs.end())
                        if (leftToken == "input")
                            jobs[leftToken] = std::make_pair(INPUT, label);
                        else if (leftToken == "output")
                            jobs[leftToken] = std::make_pair(OUTPUT, label);
                        else
                            jobs[leftToken] = std::make_pair(JOB, label);
                }
                // Skip over the > symbol
                j++;
            }
            // Invalid character
            else if (c != ' ' && c != '{' && c != '}')
            {
                generateError(1, "Unknown symbol", lineNumber);
                return;
            }
            // std::cout << leftToken << "|" << rightToken << std::endl;
        }

        // STATE DEPENDENCY HERE TOO
        if (rightToken != "")
        {
            // Error
            if (rightToken == "input")
            {
                generateError(1, "Input block cannot have a dependency", lineNumber);
                return;
            }
            if (leftToken == "output" && rightToken != "")
            {
                generateError(1, "Output block cannot be a dependency", lineNumber);
                return;
            }
            // std::cout << rightToken << " depends on " << leftToken << std::endl;
            if (jobs.find(rightToken) == jobs.end())
                if (rightToken == "output")
                    jobs[rightToken] = std::make_pair(OUTPUT, label);
                else
                    jobs[rightToken] = std::make_pair(JOB, label);

            temp.from = leftToken;
            temp.to = rightToken;
            connections.push_back(temp);
        }

        // Lastly, check for just job declaration
        if (rightToken != "")
        {
            // Add to list of jobs
            if (jobs.find(rightToken) == jobs.end())
                if (rightToken == "input")
                    jobs[rightToken] = std::make_pair(INPUT, label);
                else if (rightToken == "output")
                    jobs[rightToken] = std::make_pair(OUTPUT, label);
                else
                    jobs[rightToken] = std::make_pair(JOB, label);
        }
        else if (leftToken != "")
        {
            // IGNORE DIGRAPH
            if (leftToken != "digraph")
                // Add to list of jobs
                if (jobs.find(leftToken) == jobs.end())
                    if (leftToken == "input")
                        jobs[leftToken] = std::make_pair(INPUT, label);
                    else if (leftToken == "output")
                        jobs[leftToken] = std::make_pair(OUTPUT, label);
                    else
                        jobs[leftToken] = std::make_pair(JOB, label);
        }
    }

    // for (int i = 0; i < connections.size(); i++)
    // {
    //     std::cout << "--------------" << std::endl;
    //     std::cout << "From: " << connections[i].from << std::endl;
    //     std::cout << "To: " << connections[i].to << std::endl;
    //     std::cout << "Shape: " << connections[i].shape << std::endl;
    //     std::cout << "Label: " << connections[i].label << std::endl;
    //     std::cout << "Style: " << connections[i].style << std::endl;
    //     std::cout << std::endl;
    // }
}

std::string Interpreter::run()
{
    // For every key and value in the jobs map
    for (auto kv : jobs)
    {
        std::string name = kv.first, label = kv.second.second;
        int var_det = kv.second.first;
        // std::cout << "-------------------" << std::endl;
        // std::cout << "Name: " << name << std::endl;
        // std::cout << "Var_det: " << var_det << std::endl;
        // std::cout << "Condition (label): " << label << std::endl;
        Node *node;

        if (var_det == INPUT)
        {
            node = new InputNode(findFrom(name)[0].to);
            nodes[name] = node;
        }
        else if (var_det == OUTPUT)
        {
            node = new OutputNode();
            nodes[name] = node;
        }
        else if (var_det == JOB)
        {
            node = new JobNode(&js, name, findFrom(name)[0].to);
            nodes[name] = node;
        }
        else if (var_det == IF_STATEMENT)
        {
            // Find true and false pointers
            std::string true_ptr = "", false_ptr = "";
            std::vector<connection> temp = findFrom(name);
            for (auto t : temp)
            {
                if (t.label == "true")
                    true_ptr = t.to;
                if (t.label == "false")
                    false_ptr = t.to;
            }
            node = new IfNode(label, true_ptr, false_ptr);
            nodes[name] = node;
        }
        else if (var_det == SWITCH)
        {
        }
        else if (var_det == MULTI)
        {
        }
    }

    // Find the input node and execute
    for (auto kv : nodes)
    {
        if (kv.first == "input")
        {
            this->output = kv.second->execute(input);
        }
    }

    return this->output;
}