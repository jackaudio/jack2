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

   $Id: hdsp.h,v 1.3 2005/11/23 11:24:29 letz Exp $
*/

#ifndef __jack_hdsp_h__
#define __jack_hdsp_h__

#include <sys/time.h>

typedef struct
{
    alsa_driver_t *driver;
}
hdsp_t;

#ifdef __cplusplus
extern "C"
{
#endif

    jack_hardware_t *
    jack_alsa_hdsp_hw_new (alsa_driver_t *driver);

#ifdef __cplusplus
}
#endif


#endif /* __jack_hdsp_h__*/
