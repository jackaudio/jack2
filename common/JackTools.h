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

#ifndef __JackTools__
#define __JackTools__

#ifdef WIN32
	#include <windows.h>
#else
	#include <sys/types.h>
	#include <unistd.h>
	#include <dirent.h>
#endif

namespace Jack
{

	struct JackTools {
	
		static int GetPID();
		static int GetUID();
		
		static char* UserDir();
		static char* ServerDir(const char* server_name, char* server_dir);
		static char* DefaultServerName();
		static void CleanupFiles(const char* server_name);

	};
}

#endif
