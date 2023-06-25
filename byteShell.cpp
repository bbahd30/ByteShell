#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <cstring>
#include <map>
#include <readline/readline.h>
#include <readline/history.h>
using namespace std;

vector<string> history;
int history_index = 0;

int changeDirectory(vector<string> args);
int showHistory(vector<string> args);
int exitProgram(vector<string> args);
int background(vector<string> args);

map<string, int (*)(vector<string>)> builtins = {
    {"cd", changeDirectory},
    {"exit", exitProgram},
    {"history", showHistory},
    {"bg", background}};

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
        cout << "Error: Expected argument to \"cd\"" << endl;
    else if (chdir(args[1].c_str()) != 0)
        perror("ByteShellError");
    return 1;
}

string command(vector<string> args)
{
    string command = "";
    for (auto arg : args)
    {
        command += arg;
        command += " ";
    }
    return command;
}

int showHistory(vector<string> args)
{
    cout << "Showing history\n";
    for (auto &command : history)
        cout << command << endl;
    return 1;
}

int background(vector<string> args)
{
    cout << "Bg";
    return 1;
}

void handleSignal(int signal)
{
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
        cout << "Background process with PID " << pid << " completed" << endl;
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
            int status;
            waitpid(pid, &status, 0);
        }
        else
        {
            signal(SIGCHLD, handleSignal);
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

    for (auto &builtin : builtins)
        if (args[0] == builtin.first)
            return builtin.second(args);

    int argsCount = args.size();
    bool backgroundProcess = (argsCount > 1 && args[argsCount - 1] == "&");
    if (backgroundProcess)
    {
        args.pop_back();
        cout << "Initializing background process\n";
    }
    return launchExtProgram(args, backgroundProcess);
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
