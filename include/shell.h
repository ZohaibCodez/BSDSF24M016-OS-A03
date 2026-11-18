#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>

#define MAX_LEN 512
#define MAXARGS 64
#define ARGLEN 30
#define PROMPT "FCIT> "

// Function prototypes
char** tokenize(char* cmdline);
int execute(char** arglist, int background);

int handle_builtin(char** arglist);

#define HISTORY_SIZE 20

extern char* history[HISTORY_SIZE];
extern int history_count;

void add_to_history(const char* cmdline);
void print_history(void);
char* history_lookup(const char* cmdline);

#define MAX_JOBS 32

typedef struct {
    pid_t pid;
    char  cmd[128];
} Job;

extern Job jobs[MAX_JOBS];
extern int job_count;

void add_job(pid_t pid, char** arglist);
void reap_jobs(void);
void print_jobs(void);

#endif // SHELL_H
