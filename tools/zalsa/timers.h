// ----------------------------------------------------------------------------
//
//  Copyright (C) 2012-2018 Fons Adriaensen <fons@linuxaudio.org>
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


#ifndef __TIMERS_H
#define __TIMERS_H


#include <math.h>
#include <sys/time.h>
#include <jack/jack.h>


#define tjack_mod ldexp (1e-6f, 32)



inline double tjack_diff (double a, double b)    
{
    double d, m;

    d = a - b;
    m = tjack_mod;
    while (d < -m / 2) d += m;
    while (d >= m / 2) d -= m;
    return d;
}


inline double tjack (jack_time_t t, double dt = 0)
{
    int32_t u = (int32_t)(t & 0xFFFFFFFFLL);
    return 1e-6 * u;
}


#endif
