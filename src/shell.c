#include "shell.h"

char* read_cmd(char* prompt, FILE* fp) {
    printf("%s", prompt);
    char* cmdline = (char*) malloc(sizeof(char) * MAX_LEN);
    int c, pos = 0;

    while ((c = getc(fp)) != EOF) {
        if (c == '\n') break;
        cmdline[pos++] = c;
    }

    if (c == EOF && pos == 0) {
        free(cmdline);
        return NULL; // Handle Ctrl+D
    }
    
    cmdline[pos] = '\0';
    return cmdline;
}

char** tokenize(char* cmdline) {
    // Edge case: empty command line
    if (cmdline == NULL || cmdline[0] == '\0' || cmdline[0] == '\n') {
        return NULL;
    }

    char** arglist = (char**)malloc(sizeof(char*) * (MAXARGS + 1));
    for (int i = 0; i < MAXARGS + 1; i++) {
        arglist[i] = (char*)malloc(sizeof(char) * ARGLEN);
        bzero(arglist[i], ARGLEN);
    }

    char* cp = cmdline;
    char* start;
    int len;
    int argnum = 0;

    while (*cp != '\0' && argnum < MAXARGS) {
        while (*cp == ' ' || *cp == '\t') cp++; // Skip leading whitespace
        
        if (*cp == '\0') break; // Line was only whitespace

        start = cp;
        len = 1;
        while (*++cp != '\0' && !(*cp == ' ' || *cp == '\t')) {
            len++;
        }
        strncpy(arglist[argnum], start, len);
        arglist[argnum][len] = '\0';
        argnum++;
    }

    if (argnum == 0) { // No arguments were parsed
        for(int i = 0; i < MAXARGS + 1; i++) free(arglist[i]);
        free(arglist);
        return NULL;
    }

    arglist[argnum] = NULL;
    return arglist;
}

// Returns 1 if arglist[0] was a built-in (and was handled here), else 0.
int handle_builtin(char** arglist) {
    if (strcmp(arglist[0], "exit") == 0) {
        printf("Shell exited.\n");
        exit(0);
    }

    if (strcmp(arglist[0], "cd") == 0) {
        if (arglist[1] == NULL) {
            fprintf(stderr, "cd: missing directory\n");
        } else if (chdir(arglist[1]) != 0) {
            perror("cd");   // e.g. "cd: No such file or directory"
        }
        return 1;
    }

    if (strcmp(arglist[0], "help") == 0) {
        printf("Built-in commands:\n");
        printf("  exit        - terminate the shell\n");
        printf("  cd <dir>    - change the current directory\n");
        printf("  help        - show this help message\n");
        printf("  jobs        - list background jobs (not implemented yet)\n");
        printf("  history     - show recent commands (!n re-runs command n)\n");
        return 1;
    }

    if (strcmp(arglist[0], "jobs") == 0) {
        printf("Job control not yet implemented.\n");
        return 1;
    }

    if (strcmp(arglist[0], "history") == 0) {
        print_history();
        return 1;
    }

    return 0;   // not a built-in
}


// ---------- Command history ----------
char* history[HISTORY_SIZE];   // stored command strings
int history_count = 0;         // how many are stored right now

// Save a copy of cmdline. If the list is full, drop the oldest one.
void add_to_history(const char* cmdline) {
    if (cmdline == NULL || cmdline[0] == '\0') return;   // ignore empty lines

    if (history_count == HISTORY_SIZE) {
        free(history[0]);                                // forget the oldest
        for (int i = 1; i < HISTORY_SIZE; i++)
            history[i - 1] = history[i];                 // shift everything left
        history_count--;
    }
    history[history_count++] = strdup(cmdline);
}

// Print the stored commands, numbered from 1.
void print_history(void) {
    for (int i = 0; i < history_count; i++)
        printf("%d  %s\n", i + 1, history[i]);
}

// cmdline looks like "!3". Return a NEW string (copy of command 3), or NULL on error.
char* history_lookup(const char* cmdline) {
    char* end;
    long n = strtol(cmdline + 1, &end, 10);              // read the number after '!'

    if (end == cmdline + 1 || *end != '\0') {            // no digits, or extra junk
        fprintf(stderr, "!: usage: !<number>\n");
        return NULL;
    }
    if (n < 1 || n > history_count) {
        fprintf(stderr, "!%ld: event not found\n", n);
        return NULL;
    }
    return strdup(history[n - 1]);
}
