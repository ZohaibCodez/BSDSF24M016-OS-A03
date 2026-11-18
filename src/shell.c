#include "shell.h"



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
        printf("  jobs        - list running background jobs\n");
        printf("  history     - show recent commands (!n re-runs command n)\n");
        return 1;
    }

    if (strcmp(arglist[0], "jobs") == 0) {
        print_jobs();
        return 1;
    }

    if (strcmp(arglist[0], "history") == 0) {
        print_history();
        return 1;
    }

    if (strcmp(arglist[0], "set") == 0) {
        print_variables();
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


// ---------- Tokenizer ----------
// Splits a line into words. Rules:
//   * words are separated by spaces/tabs
//   * '<', '>' and '|' are always their own token (even without spaces)
//   * "double" or 'single' quotes keep spaces inside one word (quotes are removed)
// Returns a NULL-terminated array of malloc'd strings, or NULL for an empty line.


// ---------- Tokenizer ----------
// Splits a line into words. Rules:
//   * words are separated by spaces/tabs
//   * '<', '>' and '|' are always their own token (even without spaces)
//   * "double" or 'single' quotes keep spaces inside one word (quotes are removed)
// Returns a NULL-terminated array of malloc'd strings, or NULL for an empty line.


// ---------- Tokenizer ----------
// Splits a line into words. Rules:
//   * words are separated by spaces/tabs
//   * '<', '>' and '|' are always their own token (even without spaces)
//   * "double" or 'single' quotes keep spaces inside one word (quotes are removed)
// Returns a NULL-terminated array of malloc'd strings, or NULL for an empty line.
char** tokenize(char* cmdline) {
    if (cmdline == NULL || cmdline[0] == '\0') return NULL;

    char** arglist = malloc(sizeof(char*) * (MAXARGS + 1));
    char* buf = malloc(strlen(cmdline) + 1);    // a token can never be longer than the line
    char* cp = cmdline;
    int argnum = 0;

    while (*cp != '\0' && argnum < MAXARGS) {
        while (*cp == ' ' || *cp == '\t') cp++;          // skip spaces
        if (*cp == '\0') break;

        int len = 0;
        if (*cp == '<' || *cp == '>' || *cp == '|' || *cp == '&') {    // operator = one-character token
            buf[len++] = *cp++;
        } else {                                         // normal word
            char quote = 0;                              // 0 = not inside quotes
            while (*cp != '\0') {
                if (quote) {
                    if (*cp == quote) quote = 0;         // closing quote
                    else buf[len++] = *cp;
                } else if (*cp == '"' || *cp == '\'') {
                    quote = *cp;                         // opening quote
                } else if (*cp == ' ' || *cp == '\t' ||
                           *cp == '<' || *cp == '>' || *cp == '|' || *cp == '&') {
                    break;                               // end of word
                } else {
                    buf[len++] = *cp;
                }
                cp++;
            }
        }
        buf[len] = '\0';
        arglist[argnum++] = strdup(buf);
    }
    free(buf);

    if (argnum == 0) {          // line had only spaces
        free(arglist);
        return NULL;
    }
    arglist[argnum] = NULL;
    return arglist;
}
