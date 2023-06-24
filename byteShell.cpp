#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <cstring>
#include <map>
using namespace std;

vector<string> history;
int history_index = 0;

int changeDirectory(char **args);
int showHistory(char **args);
int exit(char **args);

map<std::string, int (*)(char **)> builtins = {
    {"cd", changeDirectory},
    {"exit", exit},
    {"history", showHistory}};

vector<char *> split(const string &str, char delimiter)
{
    vector<string> tokens;
    string token;
    istringstream tokenStream(str);
    while (getline(tokenStream, token, delimiter))
        tokens.push_back(token);
    vector<char *> args;
    for (auto &token : tokens)
        args.push_back(const_cast<char *>(token.c_str()));
    args.push_back(nullptr);
    return args;
}

int changeDirectory(char **args)
{
    cout << "Changing Directory\n";
    if (args[1] == NULL)
        cout << "Error: Expected argument to \"cd\"" << endl;
    else if (chdir(args[1]) != 0)
        perror("ByteShellError");
    return 1;
}

int showHistory(char **args)
{
    cout << "Showing history\n";
    for (const string &command : history)
        cout << command << endl;
    return 1;
}

int launchExtProgram(char **args)
{
    pid_t pid = fork();
    if (pid == 0)
    {
        if (execvp(args[0], args) == -1)
            perror("ByteShellError");
        exit(EXIT_FAILURE);
    }
    else if (pid < 0)
        perror("ByteShellError");
    else
    {
        int status;
        waitpid(pid, &status, 0);
        return 1;
    }
    return -1;
}

int execute(char **args)
{
    if (args[0] == NULL)
        return 1;

    for (auto &builtin : builtins)
        if (args[0] == builtin.first)
            return builtin.second(args);

    return launchExtProgram(args);
}

int exit(char **args)
{
    return -1;
}

int main()
{
    string line;
    int status;

    do
    {
        cout << "> ";
        getline(cin, line);
        if (!line.empty())
            history.push_back(line);
        history_index = history.size();
        vector<char *> args = split(line, ' ');
        status = execute(args.data());
    } while (status != -1);

    return 0;
}
