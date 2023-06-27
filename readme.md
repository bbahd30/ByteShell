## Instructions to run

The ByteShell uses the library readline for arrow based commands display and tab completion. Hence follow these commands to run:

 

Clone the repository.

```cpp
git clone https://github.com/bbahd30/ByteShell.git
```

Install the readline development libraries

```cpp
sudo apt-get install libreadline-dev
```

Compile the code in the directory of ByteShell

```cpp
g++ byteShell.cpp -lreadline
```

Run the executable file and run commands.

### Shell Built-ins

- Commands that are built into the shell itself rather than being an external program or utility.
- They have direct access to the internal features and data structures of the shell. This allows them to manipulate shell variables, control flow, and perform other operations that are specific to the shell environment.

## Implemented Shell Built-ins

### `kill` command

- This is used to terminate processes manually.
- It sends a signal to a process that terminates the process.
- This has been implemented by using the `kill()` system call.
    
    <aside>
    📌 The **kill**() system call can send any signal to any process group or process.
    
    </aside>
    
    - `kill(pid, SIGTERM)` ⇒ signals it to terminate the process with specified process ID.

### `jobs` command

- `jobs` command is **used to list the jobs that you are running in the background and in the foreground**.
- This is handled by creating a `struct` as shown.
    
    ```cpp
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
    ```
    

### `bg` command

- Known as the ‘**background**’ command
- The function of this command is to send a service, which is working in the foreground or a stopped process, to the background using job control.
- This is done with the help of the shell and the operating system.
    
    
    | Option | Effect |
    | --- | --- |
    | %n | Sends the job, having Job ID ‘n’, to the background. |
    | %% or %+ | Sends the current job to the background. |
    | %- | Sends the previous job to the background. |
    | %string | Sends the job whose name starts with string to the background. |
- Ways to start process
    - Foreground Processes
        - every process that starts run in the foreground
        - no other commands can be run (start any other processes) because the prompt will not be available until the program finishes processing and comes out.
    - Background Processes
        - also allow running other commands and waits if it needs input from the user.
        - the symbol `&` at the end of command signals the command to run as a background process
            - in case, if the process is suspended by the user (using Ctrl + Z), which is visible when used `ps` or `jobs`, the `bg` command can be used alternatively
            - `bg` moves a suspended or stopped job to the background, allowing it to continue executing while the shell can accept new commands.
        
        <aside>
        📌 System calls are used to wait for state changes in a child of the calling process, and obtain information about the child whose state has changed. A state change is considered to be: the child terminated; the child was stopped by a signal; or the child was resumed by a signal.
        
        </aside>
        
        - The background process creation is handled which is indicated by `&` by passing the signal `SIGCHLD`, which is signalled using `signal` function
            
            > The **signal**() system call installs a new signal handler for the signal with number *signum*.
            > 
        - This is managed using:
            
            ```cpp
            if (!backgroundProcess)
            {
            	int status;
                waitpid(pid, &status, 0);
            }
            else
            {
            	signal(SIGCHLD, handleSignal);
                cout << "Background process created with PID: " << pid << endl;
            }
            ```
            
- When a foreground process is stopped using the `Ctrl+Z`, the process is with status of ‘stopped’ is instantiated and pushed to `jobs` vector
    - the stopped process is resumed and run in background by using the `bg` command, which is implemented by using the `kill(pid, SIGCONT)` system call, which tells that process to restart.

### `alias` command

- It is used to create shortcuts for the long commands.
- The command is checked whether it is set as an alias or not, while the command entered is examined.
    - on being set as alias, the expanded command is placed in the vector `expandedAlias` and then the rest of the arguments are added
    - which then later calls itself, with the expanded arguments.

### `unalias` command

- This removes the set alias.
- To remove all the aliases, the `-a` flag is used, which has been implement using a map.

### `cd` command

- `cd` stands for change directory.
- This is implemented by using the `chdir` system call, which takes the path as argument.

### `exit` command

- This command terminates the ByteShell.

### `history` command

- This command shows the various commands from the beginning that were executed in the current shell session.
- It uses a vector to implement the same.
### `pwd` command

- It stands for ‘print working directory’, hence showing the current directory.
- This is implemented using the `getcwd(addr, size)` function of `unistd.h` header file.
    - where `addr` is address of the first character of the string, of `size` is the size given to the string for storing the path.


## References

[ACM, IIT Roorkee](https://github.com/acmiitr)

[Unix Systems call, Tutorialspoint](https://www.tutorialspoint.com/unix_system_calls/waitpid.htm)