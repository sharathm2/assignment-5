1. Your shell forks multiple child processes when executing piped commands. How does your implementation ensure that all child processes complete before the shell continues accepting user input? What would happen if you forgot to call waitpid() on all child processes?

This implementation ensures that all child processes complete, by using waitpid() on all child processes(loop). This is done after all the commands within the pipeline are executed. The waitpid() function is called for each child process ID which is retrieved from the pids array. This ensures that the parent process waits for all child processes to finish before continuing to accept user input. If we forgot to call waitpid() on all the child processes, the shell would continue to accept user input before the child processes have completed. This could lead to processes running in the background or crashing the shell.

2. The dup2() function is used to redirect input and output file descriptors. Explain why it is necessary to close unused pipe ends after calling dup2(). What could go wrong if you leave pipes open?

After using dup2() to redirect input or output of processes to a pipe, it is necessary to close the unused pipe ends. If we leave pipe ends open, it could lead to resource leaks. Specifically, if the write end of a pipe is not closed, the readoing process will not receive an EOF signal, which can cause it to continue to wait and read for input. Not closing the read end could lead to a file descriptor leak.

3. Your shell recognizes built-in commands (cd, exit, dragon). Unlike external commands, built-in commands do not require execvp(). Why is cd implemented as a built-in rather than an external command? What challenges would arise if cd were implemented as an external process?

The cd command is implemented as a built-in command because it involves changing the current working directory of the shell. If cd were implemented as an external process, it wold change the working directory of the child process, not the shell process itself. This would mean that the shells working directory would remain the same, making the cd command ineffective.

4. Currently, your shell supports a fixed number of piped commands (CMD_MAX). How would you modify your implementation to allow an arbitrary number of piped commands while still handling memory allocation efficiently? What trade-offs would you need to consider?

To allow an arbritrary number of piped commands, we could dynamically allocate memory for the pipes and pids(process_ids) instead of using a fixed size array. This can be done using malloc() which allows us to dynamically allocate memory for arrays. Some trade-offs to consider would be the overhead of dynamic memory using malloc in order to ensure no memory leaks occur. Additionally, resizing arrays can introduce overhead as well.
