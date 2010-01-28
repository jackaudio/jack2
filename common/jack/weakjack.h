/*
    Copyright (C) 2010 Paul Davis
    
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

#ifndef __weakjack_h__
#define __weakjack_h__

#ifndef JACK_OPTIONAL_WEAK_EXPORT
/* JACK_OPTIONAL_WEAK_EXPORT needs to be a macro which
   expands into a compiler directive. If non-null, the directive 
   must tell the compiler to arrange for weak linkage of 
   the symbol it used with. For this to work fully may
   require linker arguments for the client as well.
*/
#ifdef __GNUC__
#define JACK_OPTIONAL_WEAK_EXPORT __attribute__((__weak__))
#else
/* Add other things here for non-gcc platforms */
#endif
#endif

#ifndef JACK_OPTIONAL_WEAK_DEPRECATED_EXPORT
/* JACK_OPTIONAL_WEAK_DEPRECATED_EXPORT needs to be a macro
   which expands into a compiler directive. If non-null, the directive
   must tell the compiler to arrange for weak linkage of the
   symbol it is used with AND optionally to mark the symbol
   as deprecated. For this to work fully may require
   linker arguments for the client as well.
*/
#ifdef __GNUC__
#define JACK_OPTIONAL_WEAK_DEPRECATED_EXPORT __attribute__((__weak__,__deprecated__))
#else
/* Add other things here for non-gcc platforms */
#endif
#endif

#endif /* weakjack */
