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

#ifndef WIN32
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <grp.h>
#ifdef __APPLE__
#include <AvailabilityMacros.h>
#endif
#include "JackError.h"
#endif


int
jack_group2gid(const char* group)
{
#ifdef WIN32
    return -1;
#else
    size_t buflen;
    char *buf;
    int ret;
    struct group grp;
    struct group *result;

    if (!group || !*group)
        return -1;

    ret = strtol(group, &buf, 10);
    if (!*buf)
        return ret;

/* MacOSX only defines _SC_GETGR_R_SIZE_MAX starting from 10.4 */
#if defined(__APPLE__) && MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_4
    buflen = 4096;
#else
    buflen = sysconf(_SC_GETGR_R_SIZE_MAX);
    if (buflen == -1)
        buflen = 4096;
#endif
    buf = (char*)malloc(buflen);

    while (buf && ((ret = getgrnam_r(group, &grp, buf, buflen, &result)) == ERANGE)) {
        buflen *= 2;
        buf = (char*)realloc(buf, buflen);
    }
    if (!buf)
        return -1;
    free(buf);
    if (ret || !result)
        return -1;
    return grp.gr_gid;
#endif
}

#ifndef WIN32
int
jack_promiscuous_perms(int fd, const char* path, gid_t gid)
{
	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
	if (gid >= 0) {
		if (((fd < 0) ? chown(path, -1, gid) : fchown(fd, -1, gid)) < 0) {
			jack_log("Cannot chgrp %s: %s. Falling back to permissive perms.", path, strerror(errno));
		} else {
			mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;
		}
	}
	if (((fd < 0) ? chmod(path, mode) : fchmod(fd, mode)) < 0) {
		jack_log("Cannot chmod %s: %s. Falling back to default (umask) perms.", path, strerror(errno));
		return -1;
	}
	return 0;
}
#endif
