#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <cstring>
#include <map>
#include <readline/readline.h>
#include <readline/history.h>
#include <set>
#include <unistd.h>
#include "colorize.cpp"
using namespace std;

struct Job
{
    Job(int id, pid_t pid, string command, string status = "running")
    {
        this->id = id;
        this->pid = pid;
        this->command = command;
        this->status = status;
    }
    pid_t pid;
    int id;
    string command;
    string status;
};

vector<Job> jobs;
int nextJobID = 1;

pid_t currentPID = 0;

vector<string> history;
int history_index = 0;

map<string, string> aliasList;

int changeDirectory(vector<string> args);
int showHistory(vector<string> args);
int exitProgram(vector<string> args);
int background(vector<string> args);
int foreground(vector<string> args);
int showJobs(vector<string> args);
int killProgram(vector<string> args);
int addAlias(vector<string> args);
int removeAlias(vector<string> args);
int pwd(vector<string> args);
int help(vector<string> args);

map<string, int (*)(vector<string>)> builtins = {
    {"cd", changeDirectory},
    {"exit", exitProgram},
    {"history", showHistory},
    {"bg", background},
    {"fg", foreground},
    {"jobs", showJobs},
    {"kill", killProgram},
    {"alias", addAlias},
    {"unalias", removeAlias},
    {"pwd", pwd},
    {"help", help}};

vector<string> split(const string &str, char delimiter)
{
    vector<string> tokens;
    string token;
    istringstream tokenStream(str);
    while (getline(tokenStream, token, delimiter))
        tokens.push_back(token);
    return tokens;
}

int changeDirectory(vector<string> args)
{
    cout << formatText("bold", "blue", "Changing Directory\n");
    if (args.size() < 2)
        chdir(getenv("HOME"));
    else if (chdir(args[1].c_str()) != 0)
        cout << formatText("bold", "red", "ByteShellError: Error in changing directory\n");
    return 1;
}

string command(vector<string> args)
{
    string command = "";
    for (auto it = args.begin(); it != args.end(); ++it)
    {
        command += *it;
        if (it != args.end() - 1)
            command += " ";
    }
    return command;
}

int background(vector<string> args)
{
    if (args.size() < 2)
    {
        cout << formatText("bold", "red", "ByteShellError: No job ID specified\n");
        return 1;
    }

    bool jobFound;
    if (args[1][0] == '%')
    {
        int jobID = stoi(args[1].substr(1));
        jobFound = false;
        for (auto &job : jobs)
        {
            if (jobID == job.id)
            {
                jobFound = true;
                if (job.status == "stopped")
                {
                    int status;
                    if (kill(job.pid, SIGCONT) == -1)
                        cout << formatText("bold", "red", "ByteShellError:error in resuming process\n");

                    else
                    {
                        job.status = "running";
                        cout << formatText("bold", "blue", "Resuming job: " + to_string(job.pid) + " in background.\n");
                    }
                    showJobs({""});
                }
                else
                    cout << formatText("bold", "red", "ByteShellError: process is already running\n");
                break;
            }
        }
        if (!jobFound)
            cout << formatText("bold", "red", "ByteShellError: Job ID not found\n");
    }
    else
        cout << formatText("bold", "red", "ByteShellError: : Job ID must be followed by '%'");
    return 1;
}

int foreground(vector<string> args)
{
    if (args.size() < 2)
    {
        cout << formatText("bold", "red", "ByteShellError: No job ID specified\n");
        return 1;
    }

    bool jobFound;
    if (args[1][0] == '%')
    {
        int jobID = stoi(args[1].substr(1));
        jobFound = false;
        for (auto &job : jobs)
        {
            if (jobID == job.id)
            {
                jobFound = true;
                if (job.status == "stopped")
                {
                    int status;
                    if (kill(job.pid, SIGCONT) == -1)
                        cout << formatText("bold", "red", "ByteShellError:error in resuming process\n");
                    else
                    {
                        cout << formatText("bold", "blue", "Resuming job: " + to_string(job.pid) + " in foreground.\n");
                        job.status = "running";
                        int status;
                        for (auto it = jobs.begin(); it != jobs.end(); ++it)
                        {
                            if (job.pid == it->pid)
                            {
                                jobs.erase(it);
                                break;
                            }
                        }
                        waitpid(job.pid, &status, 0);
                    }
                }
                else
                    cout << formatText("bold", "red", "ByteShellError: process is already running\n");
                break;
            }
        }
        if (!jobFound)
            cout << formatText("bold", "red", "ByteShellError: Job ID not found\n");
    }
    else
        cout << formatText("bold", "red", "ByteShellError: Job ID must be followed by '%'\n");
    return 1;
}

string getPath()
{
    string cwd("\0", FILENAME_MAX + 1);
    return getcwd(&cwd[0], cwd.capacity());
}

int pwd(vector<string> args)
{
    cout << getPath() << "\n";
    return 1;
}

int help(vector<string> args)
{
    cout << "Available built-in commands in ByteShell:" << endl;
    cout << formatText("bold", "blue", "cd <directory>") << "       Changes the current directory" << endl;
    cout << formatText("bold", "blue", "bg <job_id>") << "          Resumes a stopped background job" << endl;
    cout << formatText("bold", "blue", "fg <job_id>") << "          Brings a background job to the foreground" << endl;
    cout << formatText("bold", "blue", "cd <directory>") << "       Changes the current directory" << endl;
    cout << formatText("bold", "blue", "alias") << "                Prints and sets aliases" << endl;
    cout << formatText("bold", "blue", "unalias <alias>") << "      Removes an alias" << endl;
    cout << formatText("bold", "blue", "kill <pid>") << "           Terminates a process using pID" << endl;
    cout << formatText("bold", "blue", "history") << "              Prints command history" << endl;
    cout << formatText("bold", "blue", "pwd") << "                  Print the current working directory" << endl;
    cout << formatText("bold", "blue", "exit") << "                 Exits the shell" << endl;
    cout << formatText("bold", "blue", "jobs") << "                 Prints background jobs" << endl;
    cout << formatText("bold", "blue", "help") << "                 Prints this help message" << endl;
    return 1;
}

int addAlias(vector<string> args)
{
    if (args.size() == 1)
    {
        if (aliasList.size() > 0)
        {
            cout << formatText("bold", "blue", "Showing list of aliases\n");
            for (auto alias : aliasList)
                cout << alias.first << " = " << alias.second << endl;
            return 1;
        }
        else
            cout << "No aliases found, use 'alias <name>=<command>' to add\n";
    }
    else if (args.size() == 3)
    {
        string cmd = command(args);
        size_t pairPos = cmd.find_first_of(" ");
        auto aliasPair = cmd.substr(pairPos + 1);
        size_t pos = aliasPair.find_first_of("=");
        string alias = aliasPair.substr(0, pos);
        cmd = aliasPair.substr(pos + 2, aliasPair.length() - pos - 3);
        aliasList[alias] = cmd;
    }
    else
        cout << "Use 'alias <name>=<command>' to add\n";
    return 1;
}

vector<string> checkAlias(const string &cmd)
{
    auto cmdIt = aliasList.find(cmd);
    vector<string> expandedAlias;
    if (cmdIt != aliasList.end())
        expandedAlias = split(cmdIt->second, ' ');
    return expandedAlias;
}

int removeAlias(vector<string> args)
{
    if (args.size() == 1)
        cout << "Use: unalias [-a] <alias>\n";
    else
    {
        if (args[1] == "-a")
        {
            aliasList.clear();
            return 1;
        }
        auto it = aliasList.find(args[1]);
        if (it != aliasList.end())
            aliasList.erase(it);
        else
            cout << formatText("bold", "red", "ByteShellError: alias not found.\n");
    }
    return 1;
}

int showHistory(vector<string> args)
{
    cout << formatText("bold", "blue", "Showing history\n");
    for (auto &command : history)
        cout << command << endl;
    return 1;
}

void handleSignal(int signal)
{
    if (signal == SIGCHLD)
    {
        int status;
        pid_t pid;
        while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
        {
            for (auto it = jobs.begin(); it != jobs.end(); ++it)
            {
                if (pid == it->pid)
                {
                    cout << "Background process with PID " << pid << " completed: '" << formatText("bold", "blue", it->command) << "'\n";
                    jobs.erase(it);
                    break;
                }
            }
        }
    }
    else if (signal == SIGTSTP && currentPID != 0)
    {
        cout << "\nForeground process stopped: " << currentPID << endl;
        Job job(jobs.size() + 1, currentPID, history.back(), "stopped");
        jobs.push_back(job);
        showJobs({""});
    }
}

int launchExtProgram(vector<string> args, bool backgroundProcess)
{
    vector<char *> charArgs;
    for (auto &arg : args)
        charArgs.push_back(const_cast<char *>(arg.c_str()));
    charArgs.push_back(nullptr);

    pid_t pid = fork();
    if (pid == 0)
    {
        if (execvp(charArgs[0], charArgs.data()) == -1)
            cout << formatText("bold", "red", "ByteShellError: Command not found.\n");
        exit(EXIT_FAILURE);
    }
    else if (pid < 0)
        cout << formatText("bold", "red", "ByteShellError: Error in forking process\n");
    else
    {
        if (!backgroundProcess)
        {
            currentPID = pid;
            int status;
            waitpid(pid, &status, WUNTRACED);
            currentPID = 0;
        }
        else
        {
            Job job(jobs.size() + 1, pid, command(args));
            jobs.push_back(job);
            cout << "'" << formatText("bold", "blue", command(args)) << "' command initialized with PID: " << pid << endl;
            showJobs({""});
        }
        return 1;
    }
    return -1;
}

int execute(vector<string> args)
{
    if (args.empty())
        return 1;

    auto expandedAlias = checkAlias(args[0]);
    if (!expandedAlias.empty())
    {
        expandedAlias.insert(expandedAlias.end(), args.begin() + 1, args.end());
        return execute(expandedAlias);
    }
    else
    {
        for (auto &builtin : builtins)
            if (args[0] == builtin.first)
                return builtin.second(args);
    }

    int argsCount = args.size();
    bool backgroundProcess = (argsCount > 1 && args[argsCount - 1] == "&");
    if (backgroundProcess)
        args.pop_back();
    return launchExtProgram(args, backgroundProcess);
}

int showJobs(vector<string> args)
{
    if (jobs.size() > 0)
    {
        cout << "Displaying background jobs" << endl;
        for (auto &job : jobs)
            cout << "[" << job.id << "] "
                 << "\t" << job.status << " " << job.command << endl;
    }
    else
        cout << "No background jobs.\n";
    return 1;
}

int killProgram(vector<string> args)
{
    if (args.size() == 0)
    {
        cout << "Usage: kill <pid>" << endl;
        return 1;
    }
    int pid = stoi(args[1]);
    if (kill(pid, SIGKILL) == 0)
    {
        for (auto it = jobs.begin(); it != jobs.end(); ++it)
        {
            if (pid == it->pid)
            {
                jobs.erase(it);
                break;
            }
        }
    }
    else
        cout << formatText("bold", "red", "ByteShellError: Error in killing process\n");

    return 1;
}

int exitProgram(vector<string> args)
{
    return -1;
}

int main()
{
    string line;
    int status;

    using_history();

    signal(SIGCHLD, handleSignal);
    signal(SIGTSTP, handleSignal);

    do
    {
        cout << formatText("bold", "cyan", "BBAHD's ByteShell:");
        cout << formatText("bold", "green", getPath());
        char *input = readline(formatText("bold", "green", "$ ").c_str());
        line = input;
        free(input);
        if (!line.empty())
            history.push_back(line);
        history_index = history.size();
        add_history(line.c_str());
        vector<string> args = split(line, ' ');
        status = execute(args);
    } while (status != -1);

    return 0;
}
