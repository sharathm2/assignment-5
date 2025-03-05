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

/**
 * change_directory - Implements the cd builtin command
 * @cmd: Pointer to the command buffer structure
 *
 * When called with no arguments, cd does nothing.
 * When called with one argument, changes directory to the specified path.
 *
 * Returns: OK on success, ERR_EXEC_CMD on failure
 */
int change_directory(cmd_buff_t *cmd) {
    if (cmd->argc == 1) {
        // No arguments, do nothing per requirements
        return OK;
    } else if (cmd->argc == 2) {
        // Change to the specified directory
        if (chdir(cmd->argv[1]) != 0) {
            perror("cd");
            return ERR_EXEC_CMD;
        }
    }
    return OK;
}

/**
 * match_command - Determines if a command is a builtin command
 * @input: The command string to check
 *
 * Returns: The appropriate Built_In_Cmds enum value
 */
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

/**
 * build_cmd_buff - Parses a command line into a cmd_buff structure
 * @cmd_line: The command line string to parse
 * @cmd_buff: Pointer to the cmd_buff structure to populate
 *
 * This function handles quoted arguments by preserving spaces within quotes.
 *
 * Returns: OK on success, ERR_MEMORY on memory allocation failure
 */
int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff) {
    // Trim leading spaces
    while (isspace((unsigned char)*cmd_line)) cmd_line++;
    
    // Trim trailing spaces
    char *end = cmd_line + strlen(cmd_line) - 1;
    while (end > cmd_line && isspace((unsigned char)*end)) end--;
    *(end + 1) = '\0';
    
    // Initialize cmd_buff
    cmd_buff->argc = 0;
    cmd_buff->_cmd_buffer = strdup(cmd_line);
    if (!cmd_buff->_cmd_buffer) return ERR_MEMORY;
    
    // Parse command line character by character to handle quotes
    char *p = cmd_buff->_cmd_buffer;
    
    while (*p) {
        // Skip spaces between tokens
        while (*p && isspace((unsigned char)*p)) p++;
        if (!*p) break;
        
        char *token_start;
        if (*p == '"') {
            // Handle quoted string
            p++; // Skip opening quote
            token_start = p;
            
            // Find closing quote
            while (*p && *p != '"') p++;
            
            // If we found a closing quote, terminate the token and move past it
            if (*p == '"') {
                *p = '\0';
                p++;
            }
        } else {
            // Handle normal token
            token_start = p;
            
            // Find end of token (space or end of string)
            while (*p && !isspace((unsigned char)*p)) p++;
            
            // If not at end of string, terminate token and move past it
            if (*p) {
                *p = '\0';
                p++;
            }
        }
        
        // Add token to argv array if not empty
        if (token_start[0] != '\0') {
            cmd_buff->argv[cmd_buff->argc++] = token_start;
            if (cmd_buff->argc >= CMD_ARGV_MAX - 1) break;
        }
    }
    
    // Null-terminate argv array for execvp
    cmd_buff->argv[cmd_buff->argc] = NULL;
    
    return OK;
}

/**
 * exec_local_cmd_loop - Main shell command loop
 *
 * Continuously prompts for and processes user commands until EOF or exit.
 * Handles built-in commands and executes external commands using fork/exec.
 *
 * Returns: The last command's return code
 */
int exec_local_cmd_loop() {
    char cmd_line[SH_CMD_MAX];
    cmd_buff_t cmd;
    int rc = 0;
    
    // Main command loop
    while (1) {
        // Prompt user and read input
        printf("%s", SH_PROMPT);
        if (fgets(cmd_line, SH_CMD_MAX, stdin) == NULL) {
            printf("\n");
            break;
        }
        
        // Remove trailing newline
        cmd_line[strcspn(cmd_line, "\n")] = '\0';
        
        // Check for exit command
        if (strcmp(cmd_line, EXIT_CMD) == 0) {
            exit(OK);
        }
        
        // Parse command line
        rc = build_cmd_buff(cmd_line, &cmd);
        if (rc != OK) {
            fprintf(stderr, "Error parsing command\n");
            continue;
        }
        
        // Handle empty command
        if (cmd.argc == 0) {
            printf(CMD_WARN_NO_CMD);
            free(cmd._cmd_buffer);
            continue;
        }
        
        // Check for built-in commands
        Built_In_Cmds cmd_type = match_command(cmd.argv[0]);
        if (cmd_type == BI_CMD_CD) {
            rc = change_directory(&cmd);
        } else if (cmd_type == BI_CMD_EXIT) {
            free(cmd._cmd_buffer);
            exit(OK_EXIT);
        } else if (cmd_type == BI_NOT_BI) {
            // Execute external command using fork/exec
            pid_t pid = fork();
            
            if (pid == 0) {
                // Child process
                if (execvp(cmd.argv[0], cmd.argv) == -1) {
                    perror("execvp");
                    exit(ERR_EXEC_CMD);
                }
            } else if (pid < 0) {
                // Fork failed
                perror("fork");
            } else {
                // Parent process
                int status;
                waitpid(pid, &status, 0);
                if (WIFEXITED(status)) {
                    rc = WEXITSTATUS(status);
                }
            }
        }
        
        // Free allocated memory
        free(cmd._cmd_buffer);
    }
    
    return rc;
}