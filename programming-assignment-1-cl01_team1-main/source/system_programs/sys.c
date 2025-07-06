#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include <string.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>

int main(void) {
    struct utsname uts;
    if (uname(&uts) < 0) {
        perror("uname");
        return EXIT_FAILURE;
    }

    /* Attempt to read a human-friendly OS name from /etc/os-release */
    char os_name[256] = {0};
    FILE *f = fopen("/etc/os-release", "r");
    if (f) {
        char line[256];
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "PRETTY_NAME=", 12) == 0) {
                char *start = strchr(line, '"');
                char *end   = start ? strchr(start + 1, '"') : NULL;
                if (start && end) {
                    *end = '\0';
                    strncpy(os_name, start + 1, sizeof(os_name) - 1);
                }
                break;
            }
        }
        fclose(f);
    }

    struct sysinfo si;
    if (sysinfo(&si) < 0) {
        perror("sysinfo");
        return EXIT_FAILURE;
    }

    /* Print OS / Kernel */
    if (os_name[0]) {
        printf("OS:     %s\n", os_name);
    } else {
        printf("OS:     %s %s\n", uts.sysname, uts.release);
    }
    printf("Kernel: %s\n", uts.release);
    printf("Host:   %s\n", uts.nodename);

    /* Uptime */
    long uptime = si.uptime;
    int days = uptime / 86400;
    int hrs  = (uptime % 86400) / 3600;
    int mins = (uptime % 3600) / 60;
    printf("Uptime: %d days, %02d:%02d\n", days, hrs, mins);

    /* Memory (in GB) */
    unsigned long total_mem = si.totalram * si.mem_unit;
    double total_gb = total_mem / (1024.0 * 1024.0 * 1024.0);
    printf("Memory: %.2f GB\n", total_gb);

    /* Current User */
    char *user = getlogin();
    if (!user) {
        struct passwd *pw = getpwuid(geteuid());
        user = pw ? pw->pw_name : "unknown";
    }
    printf("User:   %s\n", user);

    /* CPU Model */
    f = fopen("/proc/cpuinfo", "r");
    if (f) {
        char line[256];
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "model name", 10) == 0) {
                char *p = strchr(line, ':');
                if (p) {
                    printf("CPU:    %s", p + 2);
                }
                break;
            }
        }
        fclose(f);
    } else {
        printf("CPU:    unknown\n");
    }

    return EXIT_SUCCESS;
}
