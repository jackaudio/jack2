/*
  JACK control API

  Copyright (C) 2008 Nedko Arnaudov
  Copyright (C) 2008 Grame
  Copyright (C) 2013 Samsung Electronics

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#ifndef __JackControlAPIAndroid__
#define __JackControlAPIAndroid__

#include "JackCompilerDeps.h"

/** opaque type for sigmask object */
typedef struct jackctl_sigmask jackctl_sigmask_t;

#ifdef __cplusplus
extern "C" {
#endif

SERVER_EXPORT int
jackctl_wait_signals_and_return(
    jackctl_sigmask_t * signals);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

