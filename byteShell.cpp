#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <cstring>
#include <map>
#include <readline/readline.h>
#include <readline/history.h>
#include <set>
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
int showJobs(vector<string> args);
int killProgram(vector<string> args);
int addAlias(vector<string> args);
int removeAlias(vector<string> args);

map<string, int (*)(vector<string>)> builtins = {
    {"cd", changeDirectory},
    {"exit", exitProgram},
    {"history", showHistory},
    {"bg", background},
    {"jobs", showJobs},
    {"kill", killProgram},
    {"alias", addAlias},
    {"unalias", removeAlias}};

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
    cout << "Changing Directory\n";
    if (args.size() < 2)
        cout << "ByteShellError: Expected argument to \"cd\"" << endl;
    else if (chdir(args[1].c_str()) != 0)
        perror("ByteShellError");
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
        cout << "Error: No job ID specified\n";
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
                        perror("ByteShellError:error in resuming process\n");
                    else
                    {
                        cout << "Resuming job: " << job.pid << endl;
                        job.status = "running";
                    }
                    showJobs({""});
                }
                else
                    perror("ByteShellError: process is already running\n");
                break;
            }
        }
        if (!jobFound)
            cout << "Error: Job ID not found" << endl;
    }
    else
        cout << "Error: Job ID must be followed by '%'" << endl;
    return 1;
}

int addAlias(vector<string> args)
{
    if (args.size() == 1)
    {
        if (aliasList.size() > 0)
        {
            cout << "Showing list of aliases\n";
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
        cout << "use 'alias <name>=<command>' to add\n ";
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
            cout << "ByteShellError: alias not found.\n";
    }
    return 1;
}

int showHistory(vector<string> args)
{
    cout << "Showing history\n";
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
                    cout << "Background process with PID " << pid << " completed: " << it->command << endl;
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
            perror("ByteShellError");
        exit(EXIT_FAILURE);
    }
    else if (pid < 0)
        perror("ByteShellError");
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
            cout << "'" << command(args) << "' command initialized with PID: " << pid << endl;
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
    if (kill(pid, SIGTERM) == -1)
        perror("ByteShell");
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
        char *input = readline("> ");
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
