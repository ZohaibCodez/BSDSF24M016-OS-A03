#include "shell.h"

int main() {
    char* cmdline;
    char** arglist;

    while ((cmdline = read_cmd(PROMPT, stdin)) != NULL) {

        // Step 1: handle "!n" BEFORE tokenizing and BEFORE saving to history
        if (cmdline[0] == '!') {
            char* expanded = history_lookup(cmdline);
            free(cmdline);
            if (expanded == NULL) continue;     // error already printed
            cmdline = expanded;                 // run the old command instead
            printf("%s\n", cmdline);            // show what is being run
        }

        // Step 2: save the command in history
        add_to_history(cmdline);

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
