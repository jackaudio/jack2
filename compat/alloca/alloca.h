/*
 * alloca.h
 *
 * Replacement header for systems lacking the alloca.h header required
 * to use the alloca function.
 *
 * Copyright (C) 2018 Karl Linden <karl.j.linden@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 * USA.
 *
 */

#ifndef ALLOCA_H
# define ALLOCA_H

# if _MSC_VER
#  include <malloc.h>
#  define alloca _alloca
# endif /* _MSC_VER */

#endif /* !ALLOCA_H */
