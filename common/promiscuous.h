/*
 Copyright (C) 2014-2017 Cédric Schieli

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#ifndef __jack_gid_h__
#define __jack_gid_h__

#ifdef __cplusplus
extern "C"
{
#endif

int jack_group2gid (const char *group); /*!< Lookup gid for a UNIX group in a thread-safe way */
#ifndef WIN32
int jack_promiscuous_perms (int fd, const char *path, gid_t gid); /*!< Set promiscuous permissions on object referenced by fd and/or path */
#endif

#ifdef __cplusplus
}
#endif

#endif /* __jack_gid_h__ */


