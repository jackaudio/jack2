#ifndef __JackPosixCommon__
#define __JackPosixCommon__

#include "jslist.h"
#include "JackCompilerDeps.h"
#include "JackError.h"

#include <errno.h>
#include <time.h>
#include <unistd.h>

namespace Jack
{

struct SERVER_EXPORT JackPosixTools
{
    static void TimespecAdd(const struct timespec *left,
        const struct timespec *right, struct timespec *result);

    static void TimespecSub(const struct timespec *left,
        const struct timespec *right, struct timespec *result);

    static int TimespecCmp(const struct timespec *left,
        const struct timespec *right);
};

} // end of namespace

#endif

