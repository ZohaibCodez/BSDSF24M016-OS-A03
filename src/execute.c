#include "shell.h"

// Runs in the CHILD. Handles "<" and ">" in args:
//   open the file -> dup2 it onto stdin/stdout -> close the extra fd,
// then removes the operator and file name from args so exec only sees the command.
// Returns 0 on success, -1 on error.
static int setup_redirection(char** args) {
    int i = 0;
    while (args[i] != NULL) {
        int is_in  = (strcmp(args[i], "<") == 0);
        int is_out = (strcmp(args[i], ">") == 0);

        if (!is_in && !is_out) {        // normal word, move on
            i++;
            continue;
        }

        if (args[i + 1] == NULL) {
            fprintf(stderr, "syntax error: missing file name after '%s'\n", args[i]);
            return -1;
        }

        int fd;
        if (is_in)
            fd = open(args[i + 1], O_RDONLY);
        else
            fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);

        if (fd < 0) {
            perror(args[i + 1]);
            return -1;
        }

        dup2(fd, is_in ? STDIN_FILENO : STDOUT_FILENO);   // fd 0 or 1 now points to the file
        close(fd);                                        // the original fd is not needed

        // remove the two tokens (operator + file name) by shifting the rest left
        int j = i;
        while (args[j + 2] != NULL) {
            args[j] = args[j + 2];
            j++;
        }
        args[j] = NULL;
        args[j + 1] = NULL;
        // do not do i++ : a new token now sits at position i
    }
    return 0;
}

// Runs in the CHILD: set up redirection, then replace this process with the command.
static void run_child(char** args) {
    if (setup_redirection(args) < 0) exit(1);

    if (args[0] == NULL) {
        fprintf(stderr, "syntax error: empty command\n");
        exit(1);
    }

    execvp(args[0], args);
    perror("Command not found");        // only runs if execvp failed
    exit(1);
}

int execute(char* arglist[]) {
    // 1. Is there a pipe in the command?
    int pipe_pos = -1;
    for (int i = 0; arglist[i] != NULL; i++) {
        if (strcmp(arglist[i], "|") == 0) {
            pipe_pos = i;
            break;
        }
    }

    // 2. No pipe: normal fork-exec-wait (redirection is done inside the child)
    if (pipe_pos == -1) {
        int status;
        pid_t cpid = fork();

        if (cpid == -1) {
            perror("fork failed");
            return -1;
        }
        if (cpid == 0) run_child(arglist);

        waitpid(cpid, &status, 0);
        return 0;
    }

    // 3. Pipe:  left | right
    if (pipe_pos == 0 || arglist[pipe_pos + 1] == NULL) {
        fprintf(stderr, "syntax error near '|'\n");
        return -1;
    }
    for (int i = pipe_pos + 1; arglist[i] != NULL; i++) {
        if (strcmp(arglist[i], "|") == 0) {
            fprintf(stderr, "only one pipe is supported\n");
            return -1;
        }
    }

    int fd[2];                          // fd[0] = read end, fd[1] = write end
    if (pipe(fd) == -1) {
        perror("pipe failed");
        return -1;
    }

    pid_t left = fork();
    if (left == -1) {
        perror("fork failed");
        close(fd[0]);
        close(fd[1]);
        return -1;
    }
    if (left == 0) {                    // LEFT child: writes into the pipe
        dup2(fd[1], STDOUT_FILENO);     // stdout -> pipe write end
        close(fd[0]);                   // close BOTH original pipe ends
        close(fd[1]);
        arglist[pipe_pos] = NULL;       // cut the list here: left command only
        run_child(arglist);
    }

    pid_t right = fork();
    if (right == -1) {
        perror("fork failed");
        close(fd[0]);
        close(fd[1]);
        waitpid(left, NULL, 0);
        return -1;
    }
    if (right == 0) {                   // RIGHT child: reads from the pipe
        dup2(fd[0], STDIN_FILENO);      // stdin <- pipe read end
        close(fd[0]);
        close(fd[1]);
        run_child(&arglist[pipe_pos + 1]);
    }

    // PARENT: must close both ends too, or the reader never sees end-of-file
    close(fd[0]);
    close(fd[1]);
    waitpid(left, NULL, 0);
    waitpid(right, NULL, 0);
    return 0;
}
