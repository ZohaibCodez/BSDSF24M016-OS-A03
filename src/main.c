#include "shell.h"
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_BLOCK 32        // max lines inside one then/else block

// Skip leading spaces and remove trailing spaces (edits the string in place).
static char* trim(char* s) {
    while (*s == ' ' || *s == '\t') s++;
    char* end = s + strlen(s);
    while (end > s && (end[-1] == ' ' || end[-1] == '\t')) end--;
    *end = '\0';
    return s;
}

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

// Run ONE command (it has no ';' inside). Returns its exit status (0 = success).
static int run_command(char* line) {
    char** arglist = tokenize(line);
    if (arglist == NULL) return 0;                // empty piece, e.g. "ls ; ; pwd"

    // A '&' as the LAST token means: run in the background.
    int count = 0;
    while (arglist[count] != NULL) count++;

    int background = 0;
    if (strcmp(arglist[count - 1], "&") == 0) {
        background = 1;
        free(arglist[count - 1]);
        arglist[count - 1] = NULL;                // remove the '&' token
    }

    expand_variables(arglist);                    // replace variable names with their values

    int status = 0;
    if (arglist[0] != NULL) {                     // (line was not only "&")
        if (is_assignment(arglist[0]))
            handle_assignment(arglist[0]);        // NAME=value: the shell stores it, no fork
        else if (handle_builtin(arglist) == 0)
            status = execute(arglist, background);
    }

    // Free the memory allocated by tokenize()
    for (int i = 0; arglist[i] != NULL; i++) {
        free(arglist[i]);
    }
    free(arglist);
    return status;
}

// Run a whole line: commands separated by ';'. Returns the status of the last one.
static int run_line(char* line) {
    int status = 0;
    char* part = line;
    while (part != NULL) {
        char* next = split_at_semicolon(part);
        status = run_command(part);
        part = next;
    }
    return status;
}

// Does this line start with the word "if" ?
static int is_if_line(const char* line) {
    while (*line == ' ' || *line == '\t') line++;
    return strncmp(line, "if", 2) == 0 &&
           (line[2] == ' ' || line[2] == '\t' || line[2] == '\0');
}

// Handle:   if <command>
//           then
//             <commands>
//           else            (optional)
//             <commands>
//           fi
// 'first_line' is the "if ..." line. We read the other lines ourselves.
static void run_if_block(char* first_line) {
    char* cond = strdup(trim(trim(first_line) + 2));   // text after the word "if"

    char* then_lines[MAX_BLOCK];
    char* else_lines[MAX_BLOCK];
    int then_n = 0, else_n = 0;
    int state = 0;      // 0 = waiting for 'then', 1 = in then-block, 2 = in else-block
    int error = 0;

    if (cond[0] == '\0') {
        fprintf(stderr, "syntax error: 'if' needs a command\n");
        error = 1;
    }

    // Keep reading lines (prompt "> ") until we see "fi".
    while (1) {
        char* line = readline("> ");
        if (line == NULL) {                        // Ctrl+D before 'fi'
            if (!error) fprintf(stderr, "syntax error: missing 'fi'\n");
            error = 1;
            break;
        }
        char* t = trim(line);

        if (strcmp(t, "fi") == 0) {
            if (state == 0 && !error) {
                fprintf(stderr, "syntax error: missing 'then'\n");
                error = 1;
            }
            free(line);
            break;                                 // end of the whole if-block
        }

        if (t[0] == '\0') {
            // blank line: ignore
        } else if (state == 0) {                   // the next line MUST be 'then'
            if (strcmp(t, "then") == 0) {
                state = 1;
            } else if (!error) {
                fprintf(stderr, "syntax error: expected 'then', got '%s'\n", t);
                error = 1;
            }
        } else if (strcmp(t, "else") == 0) {
            if (state == 2 && !error) {
                fprintf(stderr, "syntax error: second 'else'\n");
                error = 1;
            }
            state = 2;
        } else if (state == 1 && then_n < MAX_BLOCK) {
            then_lines[then_n++] = strdup(t);      // store a line of the then-block
        } else if (state == 2 && else_n < MAX_BLOCK) {
            else_lines[else_n++] = strdup(t);      // store a line of the else-block
        } else if (!error) {
            fprintf(stderr, "syntax error: block is too long\n");
            error = 1;
        }
        free(line);
    }

    // Run it: first the 'if' command, then pick the block by its exit status.
    if (!error) {
        if (run_line(cond) == 0) {                 // 0 = success -> then-block
            for (int i = 0; i < then_n; i++) run_line(then_lines[i]);
        } else {                                   // non-zero = failure -> else-block
            for (int i = 0; i < else_n; i++) run_line(else_lines[i]);
        }
    }

    for (int i = 0; i < then_n; i++) free(then_lines[i]);
    for (int i = 0; i < else_n; i++) free(else_lines[i]);
    free(cond);
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

        // Step 3: an 'if' block, or normal commands split at ';'
        if (is_if_line(cmdline))
            run_if_block(cmdline);
        else
            run_line(cmdline);

        free(cmdline);
    }

    printf("\nShell exited.\n");
    return 0;
}
