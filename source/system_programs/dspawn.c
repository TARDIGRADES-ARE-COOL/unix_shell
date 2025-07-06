#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <string.h>
#include <limits.h>
#include <sys/wait.h>
#include <libgen.h>   // dirname()

static char project_dir[2048];
static char invocation_dir[2048];

static void discover_project_dir(void) {
    char exe_path[2048];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path)-1);
    if (len < 0) {
        perror("readlink");
        exit(EXIT_FAILURE);
    }
    exe_path[len] = '\0';
    char *bin_dir = dirname(exe_path);    // …/pa1/bin
    char *root_dir = dirname(bin_dir);    // …/pa1
    strncpy(project_dir, root_dir, sizeof(project_dir));
}

static int daemon_work(void) {
    char logpath[2048];
    snprintf(logpath, sizeof(logpath), "%s/dspawn.log", project_dir);

    // Initial header
    FILE *f = fopen(logpath, "a");
    if (!f) {
        perror("fopen initial log");
        return EXIT_FAILURE;
    }
    fprintf(f,
        "Daemon process running with PID: %d, PPID: %d, opening logfile: %s\n",
        getpid(), getppid(), logpath);
    fprintf(f, "Current working directory: %s\n", invocation_dir);
    fclose(f);

    // Periodic lines
    for (int i = 0; i < 5; ++i) {
        sleep(5);
        f = fopen(logpath, "a");
        if (!f) {
            perror("fopen periodic log");
            return EXIT_FAILURE;
        }
        fprintf(f, "PID %d Daemon writing line %d to the file.\n",
                getpid(), i);
        fclose(f);
    }
    return EXIT_SUCCESS;
}

int main(void) {
    // Capture invocation directory
    if (!getcwd(invocation_dir, sizeof(invocation_dir))) {
        perror("getcwd");
        exit(EXIT_FAILURE);
    }

    // Determine project directory
    discover_project_dir();

    // First fork
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); exit(EXIT_FAILURE); }
    if (pid > 0) {
        waitpid(pid, NULL, 0);
        return EXIT_SUCCESS;
    }

    // Intermediate child
    if (setsid() < 0) { perror("setsid"); exit(EXIT_FAILURE); }
    signal(SIGHUP, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);

    // Second fork
    pid = fork();
    if (pid < 0) { perror("fork"); exit(EXIT_FAILURE); }
    if (pid > 0) exit(EXIT_SUCCESS);

    // Grandchild daemon setup
    umask(0);
    chdir("/");  // detach from any dir

    // Close FDs
    for (int fd = sysconf(_SC_OPEN_MAX); fd >= 0; --fd) close(fd);
    int fd0 = open("/dev/null", O_RDWR);
    if (fd0 >= 0) { dup(fd0); dup(fd0); dup(fd0); }

    prctl(PR_SET_NAME, "dspawn_daemon", 0, 0, 0);

    exit(daemon_work());
}
