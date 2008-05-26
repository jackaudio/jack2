/*
Copyright (C) 2004-2006 Grame

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


#include "JackWinProcessSync.h"
#include "JackError.h"

namespace Jack
{

bool JackWinProcessSync::TimedWait(long usec)
{
	DWORD res = WaitForSingleObject(fEvent, usec / 1000);
	return (res == WAIT_OBJECT_0);
}
	
void JackWinProcessSync::Wait()
{
	WaitForSingleObject(fEvent, INFINITE);
}
	
} // end of namespace



