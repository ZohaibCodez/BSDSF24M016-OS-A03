#include "shell.h"

Job jobs[MAX_JOBS];     // list of running background jobs
int job_count = 0;

// Remember a new background job (its PID and its command text).
void add_job(pid_t pid, char** arglist) {
    if (job_count == MAX_JOBS) {
        fprintf(stderr, "too many background jobs (not tracking pid %d)\n", (int)pid);
        return;
    }

    Job* j = &jobs[job_count++];
    j->pid = pid;
    j->cmd[0] = '\0';

    // build the text "cmd arg1 arg2 ..." for the jobs list
    for (int i = 0; arglist[i] != NULL; i++) {
        if (i > 0) strncat(j->cmd, " ", sizeof(j->cmd) - strlen(j->cmd) - 1);
        strncat(j->cmd, arglist[i], sizeof(j->cmd) - strlen(j->cmd) - 1);
    }
}

// Collect (reap) every background child that has finished, WITHOUT waiting.
// A finished child that nobody waits for stays as a "zombie" in the process table.
void reap_jobs(void) {
    int status;
    pid_t pid;

    // WNOHANG: return 0 immediately if no child has finished yet
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < job_count; i++) {
            if (jobs[i].pid == pid) {
                printf("[done] %d  %s\n", (int)pid, jobs[i].cmd);
                for (int k = i + 1; k < job_count; k++)    // remove it from the list
                    jobs[k - 1] = jobs[k];
                job_count--;
                break;
            }
        }
    }
}

// The 'jobs' built-in.
void print_jobs(void) {
    reap_jobs();                        // first remove jobs that already finished

    if (job_count == 0) {
        printf("No background jobs.\n");
        return;
    }
    for (int i = 0; i < job_count; i++)
        printf("[%d] %d  %s\n", i + 1, (int)jobs[i].pid, jobs[i].cmd);
}
