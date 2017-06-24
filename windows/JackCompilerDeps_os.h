/*
 Copyright (C) 2004-2008 Grame

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


#ifndef __JackCompilerDeps_WIN32__
#define __JackCompilerDeps_WIN32__

#define	LIB_EXPORT __declspec(dllexport)

#ifdef SERVER_SIDE
    #define	SERVER_EXPORT __declspec(dllexport)
#else
    #define	SERVER_EXPORT
#endif

#if __GNUC__

    #define MEM_ALIGN(x,y) x __attribute__((aligned(y)))
    
#else

    #define MEM_ALIGN(x,y) x
 
#endif

#if defined(_MSC_VER) /* Added by JE - 31-01-2012 */
#define strdup _strdup
#if _MSC_VER < 1900
// This wrapper is not fully standard-compliant.  _snprintf() does not
// distinguish whether a result is truncated or a format error occurs.
inline int vsnprintf(char* buf, size_t buf_len, const char* fmt, va_list args)
{
	int str_len = _vsnprintf(buf, buf_len - 1, fmt, args);
	if (str_len == buf_len - 1 || str_len < 0) {
		buf[buf_len - 1] = '\0';
		return buf_len - 1;
	}
	return str_len;
}

inline int snprintf(char* buf, size_t buf_len, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int str_len = vsnprintf(buf, buf_len, fmt, args);
	va_end(args);
	return str_len;
}
#endif
#endif

#endif
