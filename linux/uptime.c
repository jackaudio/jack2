#include <sys/sysinfo.h>

long uptime(void) {
    struct sysinfo si;

    if (sysinfo(&si) != 0)
    {
        return -1;
    }

    return si.uptime;
}
