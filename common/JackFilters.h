/*
Copyright (C) 2008 Grame

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

*/

#ifndef __JackFilters__
#define __JackFilters__

#include "jack.h"

namespace Jack
{

    #define MAX_SIZE 64
    
	struct JackFilter 
    {
    
        jack_time_t fTable[MAX_SIZE];
        
        JackFilter()
        {
            for (int i = 0; i < MAX_SIZE; i++)
                fTable[i] = 0;
        }
        
        void AddValue(jack_time_t val)
        {
            memcpy(&fTable[1], &fTable[0], sizeof(jack_time_t) * (MAX_SIZE - 1));
            fTable[0] = val;
        }
        
        jack_time_t GetVal()
        {
            jack_time_t mean = 0;
            for (int i = 0; i < MAX_SIZE; i++)
                mean += fTable[i];
            
            return mean / MAX_SIZE;
        }
    };
    
    inline float Range(float min, float max, float val)
    {
        return (val < min) ? min : ((val > max) ? max : val);
    }

}

#endif
