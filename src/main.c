#include "shell.h"
#include <readline/readline.h>
#include <readline/history.h>

int main() {
    char* cmdline;
    char** arglist;

    // readline() prints the prompt, handles editing, Tab and arrow keys,
    // and returns a malloc'd string (or NULL on Ctrl+D).
    while ((cmdline = readline(PROMPT)) != NULL) {

        // Step 1: handle "!n" BEFORE tokenizing and BEFORE saving to history
        if (cmdline[0] == '!') {
            char* expanded = history_lookup(cmdline);
            free(cmdline);
            if (expanded == NULL) continue;     // error already printed
            cmdline = expanded;                 // run the old command instead
            printf("%s\n", cmdline);            // show what is being run
        }

        // Step 2: save the command (only if not empty)
        if (cmdline[0] != '\0') {
            add_history(cmdline);       // readline's history -> Up/Down arrows
            add_to_history(cmdline);    // our own history  -> 'history' and '!n'
        }

        // Step 3: same as before
        if ((arglist = tokenize(cmdline)) != NULL) {
            if (handle_builtin(arglist) == 0)
                execute(arglist);

            // Free the memory allocated by tokenize()
            for (int i = 0; arglist[i] != NULL; i++) {
                free(arglist[i]);
            }
            free(arglist);
        }
        free(cmdline);
    }

    printf("\nShell exited.\n");
    return 0;
}
