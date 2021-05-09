#include "JackPosixCommon.h"
#include "JackConstants.h"
#include "JackTools.h"
#include "JackError.h"
#include <assert.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

namespace Jack
{

void JackPosixTools::TimespecAdd(const struct timespec *left,
    const struct timespec *right, struct timespec *result)
{
	assert(result != NULL);
    result->tv_sec = left->tv_sec + right->tv_sec;
    result->tv_nsec = left->tv_nsec + right->tv_nsec;
    if (result->tv_nsec >= 1000000000) {
        result->tv_nsec -= 1000000000;
        result->tv_sec += 1;
    }
}

void JackPosixTools::TimespecSub(const struct timespec *left,
    const struct timespec *right, struct timespec *result)
{
    assert(result != NULL);
    result->tv_sec = left->tv_sec - right->tv_sec;
    result->tv_nsec = left->tv_nsec - right->tv_nsec;
    if (result->tv_nsec < 0) {
        result->tv_nsec += 1000000000;
        result->tv_sec -= 1;
    }
}

int JackPosixTools::TimespecCmp(const struct timespec *left,
    const struct timespec *right)
{
    if (left->tv_sec != right->tv_sec)
        return left->tv_sec < right->tv_sec ? -1 : 1;
    if (left->tv_nsec != right->tv_nsec)
        return left->tv_nsec < right->tv_nsec ? -1 : 1;
    return 0;
}

} // end of namespace


