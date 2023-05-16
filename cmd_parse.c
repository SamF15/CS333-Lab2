/* Author: Sam Fletcher
 * Program: Lab2 - PSUsh
 * Date: 16 May, 2023
 * Description: This file contains the definitions for the functions
 * prototyped in cmd_parse.h. These functions 
 * Citations:
 * Shamelessly stolen from rchaney@pdx.edu
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "cmd_parse.h"

// I have this a global so that I don't have to pass it to every
// function where I might want to use it. Yes, I know global variables
// are frowned upon, but there are a couple useful uses for them.
// This is one.
unsigned short is_verbose = 0;
// The history list is global for the same reason.
char *hist[HIST] = {'\0'};

int 
process_user_input_simple(void)
{
    char str[MAX_STR_LEN] = {'\0'};
    char *ret_val = NULL;
    char *raw_cmd = NULL;
    cmd_list_t *cmd_list = NULL;
    int cmd_count = 0;
    char prompt[PROMPT_LEN] = {'\0'};
    char hostName[HOST_NAME_MAX] = {'\0'};
    char currDir[PATH_MAX] = {'\0'};

    // Initialize history to 0
    memset(hist, 0, sizeof(hist));

    // Get host name
    if (gethostname(hostName, HOST_NAME_MAX) != 0) {
        fprintf(stderr, "Error getting host name: %d.\n", errno);
    }

    for ( ; ; ) {
        // Set up a cool user prompt.
        // test to see of stdout is a terminal device (a tty)
        sprintf(prompt, " %s %s\n%s@%s # "
                , PROMPT_STR
                , getcwd(currDir, PATH_MAX)
                , getenv("LOGNAME")
                , hostName
            );

        // If output is not a terminal, no prompt
        if(isatty(fileno(stdout))) {
            fputs(prompt, stdout);
        }

        // Disable SIGINT
        signal(SIGINT, SIG_IGN);

        // Get input off command line
        memset(str, 0, MAX_STR_LEN);
        ret_val = fgets(str, MAX_STR_LEN, stdin);

        if (NULL == ret_val) {
            // end of input, a control-D was pressed.
            // Bust out of the input loop and go home.
            break;
        }

        // STOMP on the pesky trailing newline returned from fgets().
        if (str[strlen(str) - 1] == '\n') {
            // replace the newline with a NULL
            str[strlen(str) - 1] = '\0';
        }
        if (strlen(str) == 0) {
            // An empty command line.
            // Just jump back to the prompt and fgets().
            // Don't start telling me I'm going to get cooties by
            // using continue.
            continue;
        }

        // Exit if bye command
        if (strcmp(str, BYE_CMD) == 0) {
            // Pickup your toys and go home. I just hope there are not
            // any memory leaks. ;-)
            break;
        }

        // Record command history
        free(hist[HIST - 1]);
        memmove(&(hist[1]), &(hist[0]), (HIST - 1) * sizeof(char *));
        hist[0] = strdup(str);
        
        // Basic commands are pipe delimited.
        // This is really for Stage 2.
        raw_cmd = strtok(str, PIPE_DELIM);

        cmd_list = (cmd_list_t *) calloc(1, sizeof(cmd_list_t));

        // This block should probably be put into its own function.
        cmd_count = 0;
        while (raw_cmd != NULL ) {
            cmd_t *cmd = (cmd_t *) calloc(1, sizeof(cmd_t));

            cmd->raw_cmd = strdup(raw_cmd);
            cmd->list_location = cmd_count++;

            if (cmd_list->head == NULL) {
                // An empty list.
                cmd_list->tail = cmd_list->head = cmd;
            }
            else {
                // Make this the last in the list of cmds
                cmd_list->tail->next = cmd;
                cmd_list->tail = cmd;
            }
            cmd_list->count++;

            // Get the next raw command.
            raw_cmd = strtok(NULL, PIPE_DELIM);
        }
        // Now that I have a linked list of the pipe delimited commands,
        // go through each individual command.
        parse_commands(cmd_list);

        // This is a really good place to call a function to exec the
        // the commands just parsed from the user's command line.
        exec_commands(cmd_list);

        // We (that includes you) need to free up all the stuff we just
        // allocated from the heap. That linked list of linked lists looks
        // like it will be nasty to free up, but just follow the memory.
        free_list(cmd_list);
        cmd_list = NULL;
    }

    free_history(hist);
    return(EXIT_SUCCESS);
}

void 
simple_argv(int argc, char *argv[] )
{
    int opt;

    while ((opt = getopt(argc, argv, "hv")) != -1) {
        switch (opt) {
        case 'h':
            // help
            // Show something helpful
            
            fprintf(stdout, "You must be out of your Vulcan mind if you think\n"
                    "I'm going to put helpful things in here.\n\n");
            exit(EXIT_SUCCESS);
            break;
        case 'v':
            // verbose option to anything
            // I have this such that I can have -v on the command line multiple
            // time to increase the verbosity of the output.
            is_verbose++;
            if (is_verbose) {
                fprintf(stderr, "verbose: verbose option selected: %d\n"
                        , is_verbose);
            }
            break;
        case '?':
            fprintf(stderr, "*** Unknown option used, ignoring. ***\n");
            break;
        default:
            fprintf(stderr, "*** Oops, something strange happened <%c> ... ignoring ...***\n", opt);
            break;
        }
    }
}

void 
exec_commands( cmd_list_t *cmds ) 
{
    cmd_t *cmd = cmds->head;

    if (1 == cmds->count) {
        if (!cmd->cmd) {
            // if it is an empty command, bail.
            return;
        }
        if (0 == strcmp(cmd->cmd, CD_CMD)) {
            if (0 == cmd->param_count) {
                // Just a "cd" on the command line without a target directory
                // need to cd to the HOME directory.
                char *path = getenv("HOME");
                if (NULL == path) {
                    fprintf(stderr, "Error finding home environment variable.\n");
                }
                if (chdir(path) != 0) {
                    fprintf(stderr, "Error changing directory to home: %s\n", strerror(errno));
                }
                // Is there an environment variable, somewhere, that contains
                // the HOME directory that could be used as an argument to
                // the chdir() fuction?
            }
            else {
                // try and cd to the target directory. It would be good to check
                // for errors here.
                if (0 == chdir(cmd->param_list->param)) {
                    // a happy chdir!  ;-)
                }
                else {
                    // a sad chdir.  :-(
                    fprintf(stderr, "Error changing directory: %s\n", strerror(errno));
                }
            }
        }
        else if (0 == strcmp(cmd->cmd, CWD_CMD)) {
            char str[MAXPATHLEN];

            // Fetch the Current Working Wirectory (CWD).
            // aka - get country western dancing
            getcwd(str, MAXPATHLEN); 
            printf(" " CWD_CMD ": %s\n", str);
        }
        else if (0 == strcmp(cmd->cmd, ECHO_CMD)) {
            // insert code here
            // insert code here
            // Is that an echo?
            param_t *param = cmd->param_list;
            while (NULL != param) {
                printf("%s ", param->param);
                param = param->next;
            }
            printf("\n");
        }
        else if (0 == strcmp(cmd->cmd, HISTORY_CMD)) {
            // display the history here
            int count = 0;
            for (int i = HIST - 1; i >= 0; --i) {
                if(hist[i] != NULL) {
                    ++count;
                    printf("  %2d   %s\n", count, hist[i]);
                }
            }
        }
        else {
            // A single command to create and exec
            // If you really do things correctly, you don't need a special call
            // for a single command, as distinguished from multiple commands.

            // Create an argv for the command and arguments to pass to execvp
            char **argv = calloc(cmd->param_count + 2, sizeof(char *));
            pid_t pid;
            int count = 1;
            param_t *param = cmd->param_list;
            argv[0] = strdup(cmd->cmd);
            while (NULL != param)
            {
                argv[count] = strdup(param->param);
                param = param->next;
                ++count;
            }

            // Check for failed fork
            if ((pid = fork()) < 0)
            {
                fprintf(stderr, "Forking failed: %d\n", errno);
                exit(EXIT_FAILURE);
            }
            else if (pid == 0) {
                // Reenable SIGINT
                signal(SIGINT, SIG_DFL);

                // Input redirection
                if (REDIRECT_FILE == cmd->input_src) {
                    int fd = open(cmd->input_file_name, O_RDONLY);
                    if (fd < 0) {
                        fprintf(stderr, "******* redir in failed %d *******\n", errno);
                        exit(7);
                    }
                    dup2(fd, STDIN_FILENO);
                    close(fd);
                }

                // Output redirection
                if (REDIRECT_FILE == cmd->output_dest) {
                    int fd;
                    // Chaney's creates output files with 644 permissions, this is to match that.
                    umask(0);
                    fd = open(cmd->output_file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fd < 0) {
                        fprintf(stderr, "******* redir out failed %d *******\n", errno);
                        exit(7);
                    }
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                }

                // Exec child process
                if (execvp(argv[0], argv) == -1) {
                    fprintf(stderr, "%s: command not found\n", argv[0]);
                    // Could put this in an exit handler, but I'm lazy
                    // so you get to look at this abomination.
                    free_list(cmds);
                    free_history(hist);
                    free_argv(argv, count);
                    exit(EXIT_FAILURE);
                }

            }
            else {
                int stat_loc;
                pid = wait(&stat_loc);
                if(WIFSIGNALED(stat_loc)) {
                    printf("child kill\n");
                }
            }

            // Free argv
            free_argv(argv, count);
        }
    }
    else {
        // Other things???
        // More than one command on the command line. Who'da thunk it!
        // This really falls into Stage 2.
    }
}

void
free_argv(char **argv, int count)
{
    for(int i = 0; i < count; ++i) {
    free(argv[i]);
    argv[i] = NULL;
    }
    free(argv);
}

void
free_history(char **_hist)
{
    for(int i = 0; i < HIST; i++) {
        free(_hist[i]);
        _hist[i] = NULL;
    }
}

void
free_list(cmd_list_t *cmd_list)
{
    cmd_t *curr = cmd_list->head;
    while (NULL != curr) {
        cmd_t *next = curr->next;
        free_cmd(curr);
        curr = next;
    }

    // Free the list itself
    free(cmd_list);
    cmd_list = NULL;
}

void
print_list(cmd_list_t *cmd_list)
{
    cmd_t *cmd = cmd_list->head;

    while (NULL != cmd) {
        print_cmd(cmd);
        cmd = cmd->next;
    }
}

void
free_cmd(cmd_t *cmd)
{
    // Free parameter list
    param_t *curr = cmd->param_list;
    while (NULL != curr) {
        param_t *next = curr->next;
        free(curr->param);
        curr->param = NULL;
        free(curr);
        curr = next;
    }

    // Free char *s
    free(cmd->raw_cmd);
    cmd->raw_cmd = NULL;
    free(cmd->cmd);
    cmd->cmd = NULL;
    free(cmd->input_file_name);
    cmd->input_file_name = NULL;
    free(cmd->output_file_name);
    cmd->output_file_name = NULL;

    // Delete the node
    free(cmd);
    cmd = NULL;
}

// Oooooo, this is nice. Show the fully parsed command line in a nice
// easy to read and digest format.
void
print_cmd(cmd_t *cmd)
{
    param_t *param = NULL;
    int pcount = 1;

    fprintf(stderr,"raw text: +%s+\n", cmd->raw_cmd);
    fprintf(stderr,"\tbase command: +%s+\n", cmd->cmd);
    fprintf(stderr,"\tparam count: %d\n", cmd->param_count);
    param = cmd->param_list;

    while (NULL != param) {
        fprintf(stderr,"\t\tparam %d: %s\n", pcount, param->param);
        param = param->next;
        pcount++;
    }

    fprintf(stderr,"\tinput source: %s\n"
            , (cmd->input_src == REDIRECT_FILE ? "redirect file" :
               (cmd->input_src == REDIRECT_PIPE ? "redirect pipe" : "redirect none")));
    fprintf(stderr,"\toutput dest:  %s\n"
            , (cmd->output_dest == REDIRECT_FILE ? "redirect file" :
               (cmd->output_dest == REDIRECT_PIPE ? "redirect pipe" : "redirect none")));
    fprintf(stderr,"\tinput file name:  %s\n"
            , (NULL == cmd->input_file_name ? "<na>" : cmd->input_file_name));
    fprintf(stderr,"\toutput file name: %s\n"
            , (NULL == cmd->output_file_name ? "<na>" : cmd->output_file_name));
    fprintf(stderr,"\tlocation in list of commands: %d\n", cmd->list_location);
    fprintf(stderr,"\n");
}

// Remember how I told you that use of alloca() is
// dangerous? You can trust me. I'm a professional.
// And, if you mention this in class, I'll deny it
// ever happened. What happens in stralloca stays in
// stralloca.
#define stralloca(_R,_S) {(_R) = alloca(strlen(_S) + 1); strcpy(_R,_S);}

void
parse_commands(cmd_list_t *cmd_list)
{
    cmd_t *cmd = cmd_list->head;
    char *arg;
    char *raw;

    while (cmd) {
        // Because I'm going to be calling strtok() on the string, which does
        // alter the string, I want to make a copy of it. That's why I strdup()
        // it.
        // Given that command lines should not be tooooo long, this might
        // be a reasonable place to try out alloca(), to replace the strdup()
        // used below. It would reduce heap fragmentation.
        //raw = strdup(cmd->raw_cmd);

        // Following my comments and trying out alloca() in here. I feel the rush
        // of excitement from the pending doom of alloca(), from a macro even.
        // It's like double exciting.
        stralloca(raw, cmd->raw_cmd);

        arg = strtok(raw, SPACE_DELIM);
        if (NULL == arg) {
            // The way I've done this is like ya'know way UGLY.
            // Please, look away.
            // If the first command from the command line is empty,
            // ignore it and move to the next command.
            // No need free with alloca memory.
            //free(raw);
            cmd = cmd->next;
            // I guess I could put everything below in an else block.
            continue;
        }
        // I put something in here to strip out the single quotes if
        // they are the first/last characters in arg.
        if (arg[0] == '\'') {
            arg++;
        }
        if (arg[strlen(arg) - 1] == '\'') {
            arg[strlen(arg) - 1] = '\0';
        }
        cmd->cmd = strdup(arg);
        // Initialize these to the default values.
        cmd->input_src = REDIRECT_NONE;
        cmd->output_dest = REDIRECT_NONE;

        while ((arg = strtok(NULL, SPACE_DELIM)) != NULL) {
            if (strcmp(arg, REDIR_IN) == 0) {
                // redirect stdin
                
                //
                // If the input_src is something other than REDIRECT_NONE, then
                // this is an improper command.
                //

                // If this is anything other than the FIRST cmd in the list,
                // then this is an error.

                cmd->input_file_name = strdup(strtok(NULL, SPACE_DELIM));
                cmd->input_src = REDIRECT_FILE;
                
            }
            else if (strcmp(arg, REDIR_OUT) == 0) {
                // redirect stdout
                     
                //
                // If the output_dest is something other than REDIRECT_NONE, then
                // this is an improper command.
                //

                // If this is anything other than the LAST cmd in the list,
                // then this is an error.

                cmd->output_file_name = strdup(strtok(NULL, SPACE_DELIM));
                cmd->output_dest = REDIRECT_FILE;

            }
            else {
                // add next param
                param_t *param = (param_t *) calloc(1, sizeof(param_t));
                param_t *cparam = cmd->param_list;

                cmd->param_count++;
                // Put something in here to strip out the single quotes if
                // they are the first/last characters in arg.
                if (arg[0] == '\'') {
                    arg++;
                }
                if (arg[strlen(arg) - 1] == '\'') {
                    arg[strlen(arg) - 1] = '\0';
                }
                param->param = strdup(arg);
                if (NULL == cparam) {
                    cmd->param_list = param;
                }
                else {
                    // I should put a tail pointer on this.
                    while (cparam->next != NULL) {
                        cparam = cparam->next;
                    }
                    cparam->next = param;
                }
            }
        }
        // This could overwrite some bogus file redirection.
        if (cmd->list_location > 0) {
            cmd->input_src = REDIRECT_PIPE;
        }
        if (cmd->list_location < (cmd_list->count - 1)) {
            cmd->output_dest = REDIRECT_PIPE;
        }

        // No need free with alloca memory.
        //free(raw);
        cmd = cmd->next;
    }

    if (is_verbose > 0) {
        print_list(cmd_list);
    }
}