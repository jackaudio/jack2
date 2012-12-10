/**
 * This source file is used to print out a stack-trace when your program
 * segfaults.  It is relatively reliable and spot-on accurate.
 *
 * This code is in the public domain.  Use it as you see fit, some credit
 * would be appreciated, but is not a prerequisite for usage.  Feedback
 * on it's use would encourage further development and maintenance.
 *
 * Author:  Jaco Kroon <jaco@kroon.co.za>
 *
 * Copyright (C) 2005 - 2008 Jaco Kroon
 */

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

//#define NO_CPP_DEMANGLE
#define SIGSEGV_NO_AUTO_INIT

#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <dlfcn.h>
#include <execinfo.h>
#include <errno.h>
#ifndef NO_CPP_DEMANGLE
char * __cxa_demangle(const char * __mangled_name, char * __output_buffer, size_t * __length, int * __status);
#endif

#include "jack/control.h"

#if defined(REG_RIP)
# define SIGSEGV_STACK_IA64
# define REGFORMAT "%016lx"
#elif defined(REG_EIP)
# define SIGSEGV_STACK_X86
# define REGFORMAT "%08x"
#else
# define SIGSEGV_STACK_GENERIC
# define REGFORMAT "%x"
#endif

#ifdef __APPLE__

// TODO : does not compile yet on OSX
static void signal_segv(int signum, siginfo_t* info, void*ptr) 
{}

#else

#include <ucontext.h>

static void signal_segv(int signum, siginfo_t* info, void*ptr) {
    static const char *si_codes[3] = {"", "SEGV_MAPERR", "SEGV_ACCERR"};

    size_t i;
    const char *si_code_str;
    ucontext_t *ucontext = (ucontext_t*)ptr;

#if defined(SIGSEGV_STACK_X86) || defined(SIGSEGV_STACK_IA64)
    int f = 0;
    Dl_info dlinfo;
    void **bp = 0;
    void *ip = 0;
#else
    void *bt[20];
    char **strings;
    size_t sz;
#endif

    if (signum == SIGSEGV)
    {
        jack_error("Segmentation Fault!");
    }
    else if (signum == SIGABRT)
    {
        jack_error("Abort!");
    }
    else if (signum == SIGILL)
    {
        jack_error("Illegal instruction!");
    }
    else if (signum == SIGFPE)
    {
        jack_error("Floating point exception!");
    }
    else
    {
        jack_error("Unknown bad signal catched!");
    }

    if (info->si_code >= 0 && info->si_code < 3) 
        si_code_str = si_codes[info->si_code];
    else
        si_code_str = "unknown";

    jack_error("info.si_signo = %d", signum);
    jack_error("info.si_errno = %d", info->si_errno);
    jack_error("info.si_code  = %d (%s)", info->si_code, si_code_str);
    jack_error("info.si_addr  = %p", info->si_addr);
#if !defined(__alpha__) && !defined(__ia64__) && !defined(__FreeBSD_kernel__) && !defined(__arm__) && !defined(__hppa__) && !defined(__sh__)
    for(i = 0; i < NGREG; i++)
        jack_error("reg[%02d]       = 0x" REGFORMAT, i, 
#if defined(__powerpc64__)
                ucontext->uc_mcontext.gp_regs[i]
#elif defined(__powerpc__)
                ucontext->uc_mcontext.uc_regs[i]
#elif defined(__sparc__) && defined(__arch64__)
                ucontext->uc_mcontext.mc_gregs[i]
#else
                ucontext->uc_mcontext.gregs[i]
#endif
                );
#endif /* alpha, ia64, kFreeBSD, arm, hppa */

#if defined(SIGSEGV_STACK_X86) || defined(SIGSEGV_STACK_IA64)
# if defined(SIGSEGV_STACK_IA64)
    ip = (void*)ucontext->uc_mcontext.gregs[REG_RIP];
    bp = (void**)ucontext->uc_mcontext.gregs[REG_RBP];
# elif defined(SIGSEGV_STACK_X86)
    ip = (void*)ucontext->uc_mcontext.gregs[REG_EIP];
    bp = (void**)ucontext->uc_mcontext.gregs[REG_EBP];
# endif

    jack_error("Stack trace:");
    while(bp && ip) {
        if(!dladdr(ip, &dlinfo))
            break;

        const char *symname = dlinfo.dli_sname;
#ifndef NO_CPP_DEMANGLE
        int status;
        char *tmp = __cxa_demangle(symname, NULL, 0, &status);

        if(status == 0 && tmp)
            symname = tmp;
#endif

        jack_error("% 2d: %p <%s+%u> (%s)",
                ++f,
                ip,
                symname,
                (unsigned)(ip - dlinfo.dli_saddr),
                dlinfo.dli_fname);

#ifndef NO_CPP_DEMANGLE
        if(tmp)
            free(tmp);
#endif

        if(dlinfo.dli_sname && !strcmp(dlinfo.dli_sname, "main"))
            break;

        ip = bp[1];
        bp = (void**)bp[0];
    }
#else
    jack_error("Stack trace (non-dedicated):");
    sz = backtrace(bt, 20);
    strings = backtrace_symbols(bt, sz);

    for(i = 0; i < sz; ++i)
        jack_error("%s", strings[i]);
#endif
    jack_error("End of stack trace");
    exit (-1);
}

#endif

int setup_sigsegv() {
    struct sigaction action;

    memset(&action, 0, sizeof(action));
    action.sa_sigaction = signal_segv;
#ifdef SA_SIGINFO
    action.sa_flags = SA_SIGINFO;
#endif
    if(sigaction(SIGSEGV, &action, NULL) < 0) {
        jack_error("sigaction failed. errno is %d (%s)", errno, strerror(errno));
        return 0;
    }

    if(sigaction(SIGILL, &action, NULL) < 0) {
        jack_error("sigaction failed. errno is %d (%s)", errno, strerror(errno));
        return 0;
    }

    if(sigaction(SIGABRT, &action, NULL) < 0) {
        jack_error("sigaction failed. errno is %d (%s)", errno, strerror(errno));
        return 0;
    }

    if(sigaction(SIGFPE, &action, NULL) < 0) {
        jack_error("sigaction failed. errno is %d (%s)", errno, strerror(errno));
        return 0;
    }

    return 1;
}

#ifndef SIGSEGV_NO_AUTO_INIT
static void __attribute((constructor)) init(void) {
    setup_sigsegv();
}
#endif
