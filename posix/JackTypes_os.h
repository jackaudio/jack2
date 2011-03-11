/*
  Copyright (C) 2001 Paul Davis

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2.1 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

#ifndef __JackTypes_POSIX__
#define __JackTypes_POSIX__

#include <stdint.h>
#include <pthread.h>

typedef unsigned long long UInt64;
typedef pthread_key_t jack_tls_key;

typedef pthread_t jack_native_thread_t;

typedef int (*jack_thread_creator_t)(pthread_t*, const pthread_attr_t*, void* (*function)(void*), void* arg);

#endif
