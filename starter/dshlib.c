#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>

#include "dshlib.h"

/*
 * Implement your exec_local_cmd_loop function by building a loop that prompts the 
 * user for input.  Use the SH_PROMPT constant from dshlib.h and then
 * use fgets to accept user input.
 * 
 *      while(1){
 *        printf("%s", SH_PROMPT);
 *        if (fgets(cmd_buff, ARG_MAX, stdin) == NULL){
 *           printf("\n");
 *           break;
 *        }
 *        //remove the trailing \n from cmd_buff
 *        cmd_buff[strcspn(cmd_buff,"\n")] = '\0';
 * 
 *        //IMPLEMENT THE REST OF THE REQUIREMENTS
 *      }
 * 
 *   Also, use the constants in the dshlib.h in this code.  
 *      SH_CMD_MAX              maximum buffer size for user input
 *      EXIT_CMD                constant that terminates the dsh program
 *      SH_PROMPT               the shell prompt
 *      OK                      the command was parsed properly
 *      WARN_NO_CMDS            the user command was empty
 *      ERR_TOO_MANY_COMMANDS   too many pipes used
 *      ERR_MEMORY              dynamic memory management failure
 * 
 *   errors returned
 *      OK                     No error
 *      ERR_MEMORY             Dynamic memory management failure
 *      WARN_NO_CMDS           No commands parsed
 *      ERR_TOO_MANY_COMMANDS  too many pipes used
 *   
 *   console messages
 *      CMD_WARN_NO_CMD        print on WARN_NO_CMDS
 *      CMD_ERR_PIPE_LIMIT     print on ERR_TOO_MANY_COMMANDS
 *      CMD_ERR_EXECUTE        print on execution failure of external command
 * 
 *  Standard Library Functions You Might Want To Consider Using (assignment 1+)
 *      malloc(), free(), strlen(), fgets(), strcspn(), printf()
 * 
 *  Standard Library Functions You Might Want To Consider Using (assignment 2+)
 *      fork(), execvp(), exit(), chdir()
 */

//CD Function to change directory
int change_directory(cmd_buff_t *cmd) {
    if (cmd->argc == 1) {
        return OK;
    } else if (cmd->argc == 2) {
        //If correct num of args passed, cd to specified path
        if (chdir(cmd->argv[1]) != 0) {
            perror("cd");
            return ERR_EXEC_CMD;
        }
    }
    return OK;
}

//Function to match command with built in commands located in dshlib.h
Built_In_Cmds match_command(const char *input) {
    if (!input) return BI_NOT_BI;
    
    if (strcmp(input, "exit") == 0) {
        return BI_CMD_EXIT;
    } else if (strcmp(input, "cd") == 0) {
        return BI_CMD_CD;
    } else if (strcmp(input, "dragon") == 0) {
        return BI_CMD_DRAGON;
    }
    
    return BI_NOT_BI;
}

//Function to parse command line input
int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff) {
    //Trim leading whitespace
    while (isspace((unsigned char)*cmd_line)) 
        cmd_line++; 
    //Trim trailing whitespace
    char *end = cmd_line + strlen(cmd_line) - 1;
    while (end > cmd_line && isspace((unsigned char)*end)) end--;
    *(end + 1) = '\0';

    //Duplicate the trimmed command line into our buffer
    cmd_buff->_cmd_buffer = strdup(cmd_line);
    if (!cmd_buff->_cmd_buffer) 
        return ERR_MEMORY;
    cmd_buff->argc = 0;

    //Take command line input and parse into tokens
    char *p = cmd_buff->_cmd_buffer;
    char *token_start = NULL;
    while (*p) {
        //Skip any whitespace between tokens
        while (*p && isspace((unsigned char)*p))
            p++;
        if (!*p) break;

        //Check if we are at a pipe operator
        if (*p == '|') {
            //Insert the pipe token.
            cmd_buff->argv[cmd_buff->argc++] = (char *)"|";
            p++; 
            continue;
        }
        if (*p == '"') {
            //If the token starts with "", skip the opening quote
            p++;
            token_start = p;
            //Read until we hit the closing quote
            while (*p && *p != '"') {
                p++;
            }
            //If a closing quote is found, terminate the token and move past it
            if (*p == '"') {
                *p = '\0';
                p++;
            }
            cmd_buff->argv[cmd_buff->argc++] = token_start;
            continue;
        }

        //For unquoted tokens, record the start of the token and read until we hit a space or pipe
        token_start = p;
        while (*p && !isspace((unsigned char)*p) && *p != '|') {
            p++;
        }
        //If a pipe is found, terminate the token and move past it
        if (*p == '|') {
            *p = '\0';
            cmd_buff->argv[cmd_buff->argc++] = token_start;
            continue;
        } else if (*p) {  //Terminate the token if not end of string
            *p = '\0';
            cmd_buff->argv[cmd_buff->argc++] = token_start;
            p++;
        } else {  //End of string
            cmd_buff->argv[cmd_buff->argc++] = token_start;
        }
    }
    cmd_buff->argv[cmd_buff->argc] = NULL;
    return OK;
}

//Function for executing the pipeline
int execute_pipeline(cmd_buff_t *cmd) {
    //Check if any pipe symbols are present in the command
    int pipe_positions[CMD_MAX] = {0};
    int pipe_count = 0;
    int i;

    //Find all pipe symbols and count them
    for (i = 0; i < cmd->argc; i++) {
        if (strcmp(cmd->argv[i], "|") == 0) {
            pipe_positions[pipe_count++] = i;
            if (pipe_count >= CMD_MAX - 1) {
                fprintf(stderr, "Too many pipes. Maximum is %d\n", CMD_MAX - 1);
                return ERR_TOO_MANY_COMMANDS;
            }
        }
    }

    //Use BUILT_IN_CMDs to match command with built in commands
    if (pipe_count == 0) {
        Built_In_Cmds cmd_type = match_command(cmd->argv[0]);
        //If the command is CD run change_directory()
        if (cmd_type == BI_CMD_CD) {
            return change_directory(cmd);
        } else if (cmd_type == BI_CMD_EXIT) {
            printf("exiting...\n");
            free(cmd->_cmd_buffer);
            exit(OK_EXIT);
        }
        else if (cmd_type == BI_NOT_BI){
            //Fork to duplicate parent process into a new child process
            pid_t pid = fork();
            if (pid == 0) {
                //Execvp() to execute the command
                if (execvp(cmd->argv[0], cmd->argv) == -1) {
                    perror("execvp");
                    exit(ERR_EXEC_CMD);
                }
            } else if (pid < 0) {
                //Handle fork failure
                perror("fork");
                return ERR_EXEC_CMD;
            } else {
                //If fork is successful, wait for child process to finish
                int status;
                waitpid(pid, &status, 0);
                if (WIFEXITED(status)) {
                    return WEXITSTATUS(status);
                }
                return ERR_EXEC_CMD;

            }
        }
    } else {
        //If pipes are present in the command, execute the pipeline
        int num_commands = pipe_count + 1;
        int pipes[CMD_MAX - 1][2]; //Array of pipe file descriptors, one for each pipe
        pid_t pids[CMD_MAX];       //Array to store child process IDs
        
        //Loop over num of pipes and create pipes
        for (i = 0; i < pipe_count; i++) {
            if (pipe(pipes[i]) == -1) {
                perror("pipe");
                return ERR_EXEC_CMD;
            }
        }
        
        int cmd_start = 0;
        int cmd_end;
        
        for (i = 0; i < num_commands; i++) {
            cmd_end = (i < pipe_count) ? pipe_positions[i] : cmd->argc;
            
            //Fork() to duplicate parent process into a new child process
            pids[i] = fork();
            
            if (pids[i] == 0) {
                //If the child process is successfull, set up stdin
                //Use dup2() to replace stdin with the previous pipe
                if (i > 0) {
                    if (dup2(pipes[i-1][0], STDIN_FILENO) == -1) {
                        perror("dup2");
                        exit(ERR_EXEC_CMD);
                    }
                }
                
                //Uses dup2() to replace stdout with the next pipe
                if (i < pipe_count) {
                    if (dup2(pipes[i][1], STDOUT_FILENO) == -1) {
                        perror("dup2");
                        exit(ERR_EXEC_CMD);
                    }
                }
                
                //Close all pipes
                for (int j = 0; j < pipe_count; j++) {
                    close(pipes[j][0]);
                    close(pipes[j][1]);
                }
                
                char *cmd_args[CMD_ARGV_MAX];
                int cmd_argc = 0;
                
                for (int j = cmd_start; j < cmd_end; j++) {
                    cmd_args[cmd_argc++] = cmd->argv[j];
                }
                cmd_args[cmd_argc] = NULL;
                
                //Execvp() to execute the command
                if (execvp(cmd_args[0], cmd_args) == -1) {
                    perror("execvp");
                    exit(ERR_EXEC_CMD);
                }
            } else if (pids[i] < 0) {
                //If the child process fails, fork error
                perror("fork");
                
                //Close all pipes if fork fails
                for (int j = 0; j < pipe_count; j++) {
                    close(pipes[j][0]);
                    close(pipes[j][1]);
                }
                
                return ERR_EXEC_CMD;
            }
            
            cmd_start = cmd_end + 1;
        }
        
        //Close all pipes
        for (int j = 0; j < pipe_count; j++) {
            close(pipes[j][0]);
            close(pipes[j][1]);
        }
        
        //Wait for child processes to finish if fork is successful
        int status;
        int last_status = 0;
        
        for (i = 0; i < num_commands; i++) {
            waitpid(pids[i], &status, 0);
            if (WIFEXITED(status)) {
                last_status = WEXITSTATUS(status);
            }
        }
        
        return last_status;
    }
    
    return OK;
}

//Replaces main function in dsh_cli.c
//Function for continously prompting user for input, parsing the input and executing the command
//Uses fork() to create a child process to execute the command and execvp() to execute the command
int exec_local_cmd_loop() {
    char cmd_line[SH_CMD_MAX];
    cmd_buff_t cmd;
    int rc = 0;

    //Start loop prompting the user for input
    while (1) {
        printf("%s", SH_PROMPT);
        fflush(stdout);

        if (fgets(cmd_line, SH_CMD_MAX, stdin) == NULL) {
            printf("\n");
            break;
        }

        cmd_line[strcspn(cmd_line, "\n")] = '\0';

        if (strcmp(cmd_line, EXIT_CMD) == 0) {
            printf("exiting...\n");
            return OK;
        }

        rc = build_cmd_buff(cmd_line, &cmd);
        if (rc != OK) {
            fprintf(stderr, "Error parsing command\n");
            continue;
        }

        if (cmd.argc == 0) {
            printf(CMD_WARN_NO_CMD);
            free(cmd._cmd_buffer);
            continue;
        }

        rc = execute_pipeline(&cmd);
        free(cmd._cmd_buffer);
    }

    return rc;
}