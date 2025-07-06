// backup.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>

int main(void) {
    char *src = getenv("BACKUP_DIR");
    if (!src) {
        fprintf(stderr, "Error: BACKUP_DIR environment variable is not set.\n");
        return 1;
    }

    // Ensure archive/ exists
    struct stat st = {0};
    if (stat("./archive", &st) == -1) {
        if (mkdir("./archive", 0755) != 0) {
            perror("mkdir ./archive");
            return 1;
        }
    }

    // Build timestamped filename
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d%H%M%S", t);

    char zipfile[2048];
    snprintf(zipfile, sizeof(zipfile), "./archive/backup-%s.zip", timestamp);

    // 1) Print the header
    printf("Creating backup of '%s' at '%s'\n", src, zipfile);

    // 2) Fork+exec zip -r so we see zip's own output
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }
    if (pid == 0) {
        // child: invoke zip -r zipfile src
        execlp("zip", "zip", "-r", zipfile, src, (char *)NULL);
        // if execlp fails:
        perror("execlp zip");
        _exit(1);
    }

    // parent: wait and capture exit status
    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        printf("Backup created successfully.\n");
        return 0;
    } else {
        printf("Backup failed (zip exited %d).\n", WEXITSTATUS(status));
        return 1;
    }
}
