#include "shell.h"
#include <readline/readline.h>
#include <readline/history.h>

// Find the first ';' that is NOT inside quotes. Put '\0' there (this cuts the
// line in two) and return the text after it. Return NULL if there is no ';'.
static char* split_at_semicolon(char* line) {
    char quote = 0;
    for (char* p = line; *p != '\0'; p++) {
        if (quote) {
            if (*p == quote) quote = 0;
        } else if (*p == '"' || *p == '\'') {
            quote = *p;
        } else if (*p == ';') {
            *p = '\0';
            return p + 1;
        }
    }
    return NULL;
}

// Run ONE command (it has no ';' inside): tokenize, look for '&', run it.
static void run_command(char* line) {
    char** arglist = tokenize(line);
    if (arglist == NULL) return;                  // empty piece, e.g. "ls ; ; pwd"

    // A '&' as the LAST token means: run in the background.
    int count = 0;
    while (arglist[count] != NULL) count++;

    int background = 0;
    if (strcmp(arglist[count - 1], "&") == 0) {
        background = 1;
        free(arglist[count - 1]);
        arglist[count - 1] = NULL;                // remove the '&' token
    }

    if (arglist[0] != NULL) {                     // (line was not only "&")
        if (handle_builtin(arglist) == 0)
            execute(arglist, background);
    }

    // Free the memory allocated by tokenize()
    for (int i = 0; arglist[i] != NULL; i++) {
        free(arglist[i]);
    }
    free(arglist);
}

int main() {
    char* cmdline;

    while (1) {
        reap_jobs();                              // clean up finished background jobs

        cmdline = readline(PROMPT);
        if (cmdline == NULL) break;               // Ctrl+D

        // Step 1: handle "!n" BEFORE tokenizing and BEFORE saving to history
        if (cmdline[0] == '!') {
            char* expanded = history_lookup(cmdline);
            free(cmdline);
            if (expanded == NULL) continue;       // error already printed
            cmdline = expanded;                   // run the old command instead
            printf("%s\n", cmdline);              // show what is being run
        }

        // Step 2: save the WHOLE line in history (before we cut it at ';')
        if (cmdline[0] != '\0') {
            add_history(cmdline);                 // readline's history -> Up/Down arrows
            add_to_history(cmdline);              // our own history  -> 'history' and '!n'
        }

        // Step 3: run every command, one after the other, split at ';'
        char* part = cmdline;
        while (part != NULL) {
            char* next = split_at_semicolon(part);
            run_command(part);
            part = next;
        }
        free(cmdline);
    }

    printf("\nShell exited.\n");
    return 0;
}
