/*
   Copyright (C) 2001 Paul Davis

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

   $Id: driver.h,v 1.2 2005/11/23 11:24:29 letz Exp $
*/

#ifndef __jack_driver_h__
#define __jack_driver_h__

#include <pthread.h>
#include "types.h"
#include "jslist.h"
#include "driver_interface.h"

typedef float gain_t;
typedef long channel_t;

typedef	enum {
    Lock = 0x1,
    NoLock = 0x2,
    Sync = 0x4,
    NoSync = 0x8
} ClockSyncStatus;

typedef void (*ClockSyncListenerFunction)(channel_t, ClockSyncStatus, void*);

typedef struct
{
    unsigned long id;
    ClockSyncListenerFunction function;
    void *arg;
}
ClockSyncListener;

struct _jack_engine;
struct _jack_driver;

typedef int (*JackDriverAttachFunction)(struct _jack_driver *,
                                        struct _jack_engine *);
typedef int (*JackDriverDetachFunction)(struct _jack_driver *,
                                        struct _jack_engine *);
typedef int (*JackDriverReadFunction)(struct _jack_driver *,
                                      jack_nframes_t nframes);
typedef int (*JackDriverWriteFunction)(struct _jack_driver *,
                                       jack_nframes_t nframes);
typedef int (*JackDriverNullCycleFunction)(struct _jack_driver *,
        jack_nframes_t nframes);
typedef int (*JackDriverStopFunction)(struct _jack_driver *);
typedef int (*JackDriverStartFunction)(struct _jack_driver *);
typedef int	(*JackDriverBufSizeFunction)(struct _jack_driver *,
        jack_nframes_t nframes);
/*
   Call sequence summary:

     1) engine loads driver via runtime dynamic linking
	 - calls jack_driver_load
	 - we call dlsym for "driver_initialize" and execute it
     2) engine attaches to driver
     3) engine starts driver
     4) driver runs its own thread, calling
         while () {
          driver->wait ();
	  driver->engine->run_cycle ()
         }
     5) engine stops driver
     6) engine detaches from driver
     7) engine calls driver `finish' routine

     Note that stop/start may be called multiple times in the event of an
     error return from the `wait' function.
*/

typedef struct _jack_driver
{

    /* The _jack_driver structure fields are included at the beginning of
       each driver-specific structure using the JACK_DRIVER_DECL macro,
       which is defined below.  The comments that follow describe each
       common field.
      
       The driver should set this to be the interval it expects to elapse
       between returning from the `wait' function. if set to zero, it
       implies that the driver does not expect regular periodic wakeups.
     
        jack_time_t period_usecs;
     
     
       The driver should set this within its "wait" function to indicate
       the UST of the most recent determination that the engine cycle
       should run. it should not be set if the "extra_fd" argument of
       the wait function is set to a non-zero value.
     
        jack_time_t last_wait_ust;
     
     
       These are not used by the driver.  They should not be written to or
       modified in any way
     
        void *handle;
        struct _jack_internal_client *internal_client;
     
       This should perform any cleanup associated with the driver. it will
       be called when jack server process decides to get rid of the
       driver. in some systems, it may not be called at all, so the driver
       should never rely on a call to this. it can set it to NULL if
       it has nothing do do.
     
        void (*finish)(struct _jack_driver *);
     
     
       The JACK engine will call this when it wishes to attach itself to
       the driver. the engine will pass a pointer to itself, which the driver
       may use in anyway it wishes to. the driver may assume that this
       is the same engine object that will make `wait' calls until a
       `detach' call is made.
     
        JackDriverAttachFunction attach;
     
     
       The JACK engine will call this when it is finished using a driver.
     
        JackDriverDetachFunction detach;
     
     
       The JACK engine will call this when it wants to wait until the 
       driver decides that its time to process some data. the driver returns
       a count of the number of audioframes that can be processed. 
     
       it should set the variable pointed to by `status' as follows:
     
       zero: the wait completed normally, processing may begin
       negative: the wait failed, and recovery is not possible
       positive: the wait failed, and the driver stopped itself.
    	       a call to `start' will return the driver to	
    	       a correct and known state.
     
       the driver should also fill out the `delayed_usecs' variable to
       indicate any delay in its expected periodic execution. for example,
       if it discovers that its return from poll(2) is later than it
       expects it to be, it would place an estimate of the delay
       in this variable. the engine will use this to decide if it 
       plans to continue execution.
     
        JackDriverWaitFunction wait;
     
     
       The JACK engine will call this to ask the driver to move
       data from its inputs to its output port buffers. it should
       return 0 to indicate successful completion, negative otherwise. 
     
       This function will always be called after the wait function (above).
     
        JackDriverReadFunction read;
     
     
       The JACK engine will call this to ask the driver to move
       data from its input port buffers to its outputs. it should
       return 0 to indicate successful completion, negative otherwise. 
     
       this function will always be called after the read function (above).
     
        JackDriverWriteFunction write;
     
     
       The JACK engine will call this after the wait function (above) has
       been called, but for some reason the engine is unable to execute
       a full "cycle". the driver should do whatever is necessary to
       keep itself running correctly, but cannot reference ports
       or other JACK data structures in any way.
     
        JackDriverNullCycleFunction null_cycle;
     
        
       The engine will call this when it plans to stop calling the `wait'
       function for some period of time. the driver should take
       appropriate steps to handle this (possibly no steps at all).
       NOTE: the driver must silence its capture buffers (if any)
       from within this function or the function that actually
       implements the change in state.
     
        JackDriverStopFunction stop;
     
     
       The engine will call this to let the driver know that it plans
       to start calling the `wait' function on a regular basis. the driver
       should take any appropriate steps to handle this (possibly no steps
       at all). NOTE: The driver may wish to silence its playback buffers
       (if any) from within this function or the function that actually
       implements the change in state.
       
        JackDriverStartFunction start;
     
       The engine will call this to let the driver know that some client
       has requested a new buffer size.  The stop function will be called
       prior to this, and the start function after this one has returned.
     
        JackDriverBufSizeFunction bufsize;
    */

    /* define the fields here... */
#define JACK_DRIVER_DECL \
    jack_time_t period_usecs; \
    jack_time_t last_wait_ust; \
    void *handle; \
    struct _jack_client_internal * internal_client; \
    void (*finish)(struct _jack_driver *);\
    JackDriverAttachFunction attach; \
    JackDriverDetachFunction detach; \
    JackDriverReadFunction read; \
    JackDriverWriteFunction write; \
    JackDriverNullCycleFunction null_cycle; \
    JackDriverStopFunction stop; \
    JackDriverStartFunction start; \
    JackDriverBufSizeFunction bufsize;

    JACK_DRIVER_DECL			/* expand the macro */
}
jack_driver_t;

void jack_driver_init (jack_driver_t *);
void jack_driver_release (jack_driver_t *);

jack_driver_t *jack_driver_load (int argc, char **argv);
void jack_driver_unload (jack_driver_t *);

/****************************
 *** Non-Threaded Drivers ***
 ****************************/

/*
   Call sequence summary:

     1) engine loads driver via runtime dynamic linking
	 - calls jack_driver_load
	 - we call dlsym for "driver_initialize" and execute it
         - driver_initialize calls jack_driver_nt_init
     2) nt layer attaches to driver
     3) nt layer starts driver
     4) nt layer runs a thread, calling
         while () {
           driver->nt_run_ctcle();
         }
     5) nt layer stops driver
     6) nt layer detaches driver
     7) engine calls driver `finish' routine which calls jack_driver_nt_finish

     Note that stop/start may be called multiple times in the event of an
     error return from the `wait' function.
*/

struct _jack_driver_nt;

typedef int (*JackDriverNTAttachFunction)(struct _jack_driver_nt *);
typedef int (*JackDriverNTDetachFunction)(struct _jack_driver_nt *);
typedef int (*JackDriverNTStopFunction)(struct _jack_driver_nt *);
typedef int (*JackDriverNTStartFunction)(struct _jack_driver_nt *);
typedef int	(*JackDriverNTBufSizeFunction)(struct _jack_driver_nt *,
        jack_nframes_t nframes);
typedef int (*JackDriverNTRunCycleFunction)(struct _jack_driver_nt *);

typedef struct _jack_driver_nt
{
#define JACK_DRIVER_NT_DECL \
    JACK_DRIVER_DECL \
    struct _jack_engine * engine; \
    volatile int nt_run; \
    pthread_t nt_thread; \
    pthread_mutex_t nt_run_lock; \
    JackDriverNTAttachFunction nt_attach; \
    JackDriverNTDetachFunction nt_detach; \
    JackDriverNTStopFunction nt_stop; \
    JackDriverNTStartFunction nt_start; \
    JackDriverNTBufSizeFunction nt_bufsize; \
    JackDriverNTRunCycleFunction nt_run_cycle;
#define nt_read read
#define nt_write write
#define nt_null_cycle null_cycle

    JACK_DRIVER_NT_DECL
}
jack_driver_nt_t;

void jack_driver_nt_init (jack_driver_nt_t * driver);
void jack_driver_nt_finish (jack_driver_nt_t * driver);

#endif /* __jack_driver_h__ */
