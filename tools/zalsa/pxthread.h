// ----------------------------------------------------------------------------
//
//  Copyright (C) 2012 Fons Adriaensen <fons@linuxaudio.org>
//    
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// ----------------------------------------------------------------------------


#ifndef __PXTHREAD_H
#define __PXTHREAD_H


#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>


class Pxthread
{
public:

    Pxthread (void);
    virtual ~Pxthread (void);
    Pxthread (const Pxthread&);
    Pxthread& operator=(const Pxthread&);

    virtual void thr_main (void) = 0;
    virtual int  thr_start (int policy, int priority, size_t stacksize = 0);

private:
  
    pthread_t  _thrid;
};


#endif
