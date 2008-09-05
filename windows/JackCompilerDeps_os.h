/*
Copyright (C) 2004-2005 Grame

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

#ifndef __JackCompilerDeps_WIN32__
#define __JackCompilerDeps_WIN32__

#if __GNUC__
	#define MEM_ALIGN(x,y) x __attribute__((aligned(y)))
	#define	EXPORT __attribute__ ((visibility("default")))
#else 
	//#define MEM_ALIGN(x,y) __declspec(align(y)) x
	#define MEM_ALIGN(x,y) x
	#define	EXPORT __declspec(dllexport)
#endif

#endif
