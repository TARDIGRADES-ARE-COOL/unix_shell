#include <stdio.h>
#include <stdlib.h>

int main() {
    FILE *fp = popen("ps -axo comm | grep dspawn_daemon | wc -l", "r");
    if (!fp) {
        perror("popen failed");
        return 1;
    }

    int count = 0;
    fscanf(fp, "%d", &count);
    pclose(fp);

    if (count == 0)
        printf("No daemon is alive right now.\n");
    else
        printf("Live daemons: %d\n", count);

    return 0;
}
