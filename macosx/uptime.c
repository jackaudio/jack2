#include <time.h>
#include <errno.h>
#include <sys/sysctl.h>

long uptime(void)
{
    struct timeval boottime;
    size_t len = sizeof(boottime);
    int mib[2] = { CTL_KERN, KERN_BOOTTIME };
    if (sysctl(mib, 2, &boottime, &len, NULL, 0) < 0)
    {
        return -1L;
    }
    time_t bsec = boottime.tv_sec, csec = time(NULL);

    return (long) difftime(csec, bsec);
}
