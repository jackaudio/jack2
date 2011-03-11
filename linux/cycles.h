/*
  Copyright (C) 2001 Paul Davis
  Code derived from various headers from the Linux kernel

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

  $Id: cycles.h,v 1.4.2.1 2006/06/20 14:44:00 letz Exp $
*/

#ifndef __jack_cycles_h__
#define __jack_cycles_h__

/*
 * Standard way to access the cycle counter on i586+ CPUs.
 * Currently only used on SMP.
 *
 * If you really have a SMP machine with i486 chips or older,
 * compile for that, and this will just always return zero.
 * That's ok, it just means that the nicer scheduling heuristics
 * won't work for you.
 *
 * We only use the low 32 bits, and we'd simply better make sure
 * that we reschedule before that wraps. Scheduling at least every
 * four billion cycles just basically sounds like a good idea,
 * regardless of how fast the machine is.
 */

#ifdef __x86_64__

typedef unsigned long cycles_t;
extern cycles_t cacheflush_time;

static inline unsigned long get_cycles(void)
{
    unsigned int hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return (((unsigned long)hi)<<32) | ((unsigned long)lo);
}

#endif /* __x86_64__ */

#ifdef __sparc_v9__
/* rd is V9 only */
static inline unsigned long long get_cycles(void)
{
	unsigned long long res;
	__asm__ __volatile__("rd %%tick, %0" : "=r"(res));
	return res;
}
#endif /* __sparc_v9__ */

#ifdef __PPC__

/* PowerPC */

#define CPU_FTR_601			0x00000100

typedef unsigned long cycles_t;

/* For the "cycle" counter we use the timebase lower half. */

extern cycles_t cacheflush_time;

static inline cycles_t get_cycles(void)
{
    cycles_t ret = 0;

    __asm__ __volatile__(
        "98:	mftb %0\n"
        "99:\n"
        ".section __ftr_fixup,\"a\"\n"
        "	.long %1\n"
        "	.long 0\n"
        "	.long 98b\n"
        "	.long 99b\n"
        ".previous"
    : "=r" (ret) : "i" (CPU_FTR_601));
    return ret;
}

#endif /* __PPC__ */

#ifdef __i386__

typedef unsigned long long cycles_t;

extern cycles_t cacheflush_time;

#define rdtscll(val) \
     __asm__ __volatile__("rdtsc" : "=A" (val))

static inline cycles_t get_cycles (void)
{
    unsigned long long ret;

    rdtscll(ret);
    return ret;
}

#endif /* __i386__ */

/* everything else but x86, amd64, sparcv9 or ppc */
#if !defined (__PPC__) && !defined (__x86_64__) && !defined (__i386__) && !defined (__sparc_v9__)

#warning No suitable get_cycles() implementation. Returning 0 instead

typedef unsigned long long cycles_t;

static inline cycles_t get_cycles(void)
{
    return 0;
}

#endif /* everything else but x86, amd64, sparcv9 or ppc */


#endif /* __jack_cycles_h__ */
