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

#include "JackIOAdapter.h"
#include <stdio.h>

namespace Jack
{

void MeasureTable::Write(int time1, int time2, float r1, float r2, int pos1, int pos2)
{
	fTable[fCount].time1 = time1;
	fTable[fCount].time2 = time2;
	fTable[fCount].r1 = r1;
	fTable[fCount].r2 = r2;
	fTable[fCount].pos1 = pos1;
	fTable[fCount].pos2 = pos2;
	fCount++;
	if (fCount == TABLE_MAX)
		fCount--;
}
                
void MeasureTable::Save()
{
	FILE* file = fopen("JackIOAdapter.log", "w");

	for (int i = 1; i < TABLE_MAX; i++) {
		fprintf(file, "%d \t %d \t %d  \t %f \t %f \t %d \t %d \n", 
		fTable[i].delta, fTable[i+1].time1 - fTable[i].time1, 
		fTable[i+1].time2 - fTable[i].time2, 
		fTable[i].r1, fTable[i].r2, fTable[i].pos1, fTable[i].pos2);
	}

	fclose(file);
}

int JackIOAdapterInterface::Open()
{
    return 0;
}

int JackIOAdapterInterface::Close()
{
    return 0;
}
 
} // namespace
