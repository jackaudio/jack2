/*
Copyright (C) 2005 Grame  

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

#ifdef WIN32 
#pragma warning (disable : 4786)
#endif

#include "JackServerGlobals.h"
#include "JackError.h"
#include "shm.h"
#include <getopt.h>

#ifndef WIN32
	#include <dirent.h>
#endif

#define DEFAULT_TMP_DIR "/tmp"
static char* jack_tmpdir = DEFAULT_TMP_DIR;
static char* server_name = NULL;

namespace Jack
{

long JackServerGlobals::fClientCount = 0;
JackServer* JackServerGlobals::fServer = NULL;

#ifndef WIN32

static char* jack_default_server_name(void)
{
    char* server_name;
    if ((server_name = getenv("JACK_DEFAULT_SERVER")) == NULL)
        server_name = "default";
    return server_name;
}

/* returns the name of the per-user subdirectory of jack_tmpdir */
static char* jack_user_dir(void)
{
    static char user_dir[PATH_MAX] = "";

    /* format the path name on the first call */
    if (user_dir[0] == '\0') {
        snprintf(user_dir, sizeof(user_dir), "%s/jack-%d", jack_tmpdir, getuid());
    }

    return user_dir;
}

/* returns the name of the per-server subdirectory of jack_user_dir() */

static char* get_jack_server_dir(const char* toto)
{
    static char server_dir[PATH_MAX] = "";

    // format the path name on the first call
    if (server_dir[0] == '\0') {
        snprintf(server_dir, sizeof (server_dir), "%s/%s", jack_user_dir(), server_name);
    }

    return server_dir;
}

static void
jack_cleanup_files(const char *server_name)
{
    DIR *dir;
    struct dirent *dirent;
    char *dir_name = get_jack_server_dir(server_name);

    /* On termination, we remove all files that jackd creates so
     * subsequent attempts to start jackd will not believe that an
     * instance is already running.  If the server crashes or is
     * terminated with SIGKILL, this is not possible.  So, cleanup
     * is also attempted when jackd starts.
     *
     * There are several tricky issues.  First, the previous JACK
     * server may have run for a different user ID, so its files
     * may be inaccessible.  This is handled by using a separate
     * JACK_TMP_DIR subdirectory for each user.  Second, there may
     * be other servers running with different names.  Each gets
     * its own subdirectory within the per-user directory.  The
     * current process has already registered as `server_name', so
     * we know there is no other server actively using that name.
     */

    /* nothing to do if the server directory does not exist */
    if ((dir = opendir(dir_name)) == NULL) {
        return ;
    }

    /* unlink all the files in this directory, they are mine */
    while ((dirent = readdir(dir)) != NULL) {

        char fullpath[PATH_MAX];

        if ((strcmp(dirent->d_name, ".") == 0)
                || (strcmp(dirent->d_name, "..") == 0)) {
            continue;
        }

        snprintf(fullpath, sizeof (fullpath), "%s/%s", dir_name, dirent->d_name);

        if (unlink(fullpath)) {
            jack_error("cannot unlink `%s' (%s)", fullpath, strerror(errno));
        }
    }

    closedir(dir);

    /* now, delete the per-server subdirectory, itself */
    if (rmdir(dir_name)) {
        jack_error("cannot remove `%s' (%s)", dir_name,
                    strerror(errno));
    }

    /* finally, delete the per-user subdirectory, if empty */
    if (rmdir (jack_user_dir())) {
        if (errno != ENOTEMPTY) {
            jack_error("cannot remove `%s' (%s)",
                        jack_user_dir(), strerror(errno));
        }
    }
}

#endif

int JackServerGlobals::JackStart(const char* server_name, jack_driver_desc_t* driver_desc, JSList* driver_params, int sync, int temporary, int time_out_ms, int rt, int priority, int loopback, int verbose)
{
    JackLog("Jackdmp: sync = %ld timeout = %ld rt = %ld priority = %ld verbose = %ld \n", sync, time_out_ms, rt, priority, verbose);
	fServer = new JackServer(sync, temporary, time_out_ms, rt, priority, loopback, verbose, server_name);
    int res = fServer->Open(driver_desc, driver_params);
    return (res < 0) ? res : fServer->Start();
}

int JackServerGlobals::JackStop()
{
    fServer->Stop();
    fServer->Close();
    JackLog("Jackdmp: server close\n");
    delete fServer;
    JackLog("Jackdmp: delete server\n");
    return 0;
}

int JackServerGlobals::JackDelete()
{
    delete fServer;
    JackLog("Jackdmp: delete server\n");
    return 0;
}

bool JackServerGlobals::Init()
{
	if (fClientCount++ == 0) {
	
        JackLog("JackServerGlobals Init\n");
		int realtime = 0;
		int client_timeout = 0; /* msecs; if zero, use period size. */
		int realtime_priority = 10;
		int verbose_aux = 0;
		int do_mlock = 1;
		unsigned int port_max = 128;
		int loopback = 0;
		int do_unlock = 0;
		int temporary = 0;
		
		jack_driver_desc_t* driver_desc;
		const char *options = "-ad:P:uvshVRL:STFl:t:mn:p:";
		static struct option long_options[] = {
										   { "driver", 1, 0, 'd'},
										   { "verbose", 0, 0, 'v' },
										   { "help", 0, 0, 'h' },
										   { "port-max", 1, 0, 'p' },
										   { "no-mlock", 0, 0, 'm' },
										   { "name", 0, 0, 'n' },
										   { "unlock", 0, 0, 'u' },
										   { "realtime", 0, 0, 'R' },
										   { "loopback", 0, 0, 'L' },
										   { "realtime-priority", 1, 0, 'P' },
										   { "timeout", 1, 0, 't' },
										   { "temporary", 0, 0, 'T' },
										   { "version", 0, 0, 'V' },
										   { "silent", 0, 0, 's' },
										   { "sync", 0, 0, 'S' },
										   { 0, 0, 0, 0 }
									   };
		int opt = 0;
		int option_index = 0;
		int seen_driver = 0;
		char *driver_name = NULL;
		char **driver_args = NULL;
		JSList* driver_params;
		int driver_nargs = 1;
		JSList* drivers = NULL;
		int show_version = 0;
		int sync = 0;
		int rc, i;
		int ret;
		
		FILE* fp = 0;
		char filename[255];
		char buffer[255];
		int argc = 0;
		char* argv[32];
		
		snprintf(filename, 255, "%s/.jackdrc", getenv("HOME"));
		fp = fopen(filename, "r");

		if (!fp) {
			fp = fopen("/etc/jackdrc", "r");
		}
		// if still not found, check old config name for backwards compatability 
		if (!fp) {
			fp = fopen("/etc/jackd.conf", "r");
		}

		argc = 0;
		if (fp) {
			ret = fscanf(fp, "%s", buffer);
			while (ret != 0 && ret != EOF) {
				argv[argc] = (char*)malloc(64);
				strcpy(argv[argc], buffer);
				ret = fscanf(fp, "%s", buffer);
				argc++;
			}
			fclose(fp);
		}
		
		/*
		For testing
		int argc = 15;
		char* argv[] = {"jackdmp", "-R", "-v", "-d", "coreaudio", "-p", "512", "-d", "~:Aggregate:0", "-r", "48000", "-i", "2", "-o", "2" };
		*/

		opterr = 0;
		optind = 1; // Important : to reset argv parsing
		
		while (!seen_driver &&
				(opt = getopt_long(argc, argv, options, long_options, &option_index)) != EOF) {
				
			switch (opt) {

				case 'd':
					seen_driver = 1;
					driver_name = optarg;
					break;

				case 'v':
					verbose_aux = 1;
					break;

				case 'S':
					sync = 1;
					break;

				case 'n':
					server_name = optarg;
					break;

				case 'm':
					do_mlock = 0;
					break;

				case 'p':
					port_max = (unsigned int)atol(optarg);
					break;

				case 'P':
					realtime_priority = atoi(optarg);
					break;

				case 'R':
					realtime = 1;
					break;

				case 'L':
					loopback = atoi(optarg);
					break;

				case 'T':
					temporary = 1;
					break;

				case 't':
					client_timeout = atoi(optarg);
					break;

				case 'u':
					do_unlock = 1;
					break;

				case 'V':
					show_version = 1;
					break;

				default:
					fprintf(stderr, "unknown option character %c\n", optopt);
					break;
			}
		}
		
		drivers = jack_drivers_load(drivers);
		if (!drivers) {
			fprintf(stderr, "jackdmp: no drivers found; exiting\n");
			goto error;
		}

		driver_desc = jack_find_driver_descriptor(drivers, driver_name);
		if (!driver_desc) {
			fprintf(stderr, "jackdmp: unknown driver '%s'\n", driver_name);
			goto error;
		}

		if (optind < argc) {
			driver_nargs = 1 + argc - optind;
		} else {
			driver_nargs = 1;
		}

		if (driver_nargs == 0) {
			fprintf(stderr, "No driver specified ... hmm. JACK won't do"
					 " anything when run like this.\n");
			goto error;
		}

		driver_args = (char**)malloc(sizeof(char *) * driver_nargs);
		driver_args[0] = driver_name;

		for (i = 1; i < driver_nargs; i++) {
			driver_args[i] = argv[optind++];
		}

		if (jack_parse_driver_params(driver_desc, driver_nargs, driver_args, &driver_params)) {
			goto error;
		}

	#ifndef WIN32
		if (server_name == NULL)
			server_name = jack_default_server_name();
	#endif

		rc = jack_register_server(server_name, false);
		switch (rc) {
			case EEXIST:
				fprintf(stderr, "`%s' server already active\n", server_name);
				goto error;
			case ENOSPC:
				fprintf(stderr, "too many servers already active\n");
				goto error;
			case ENOMEM:
				fprintf(stderr, "no access to shm registry\n");
				goto error;
			default:
				if (jack_verbose)
					fprintf(stderr, "server `%s' registered\n", server_name);
		}

		/* clean up shared memory and files from any previous instance of this server name */
		jack_cleanup_shm();
	#ifndef WIN32
		jack_cleanup_files(server_name);
	#endif

		if (!realtime && client_timeout == 0)
			client_timeout = 500; /* 0.5 sec; usable when non realtime. */
			
		for (i = 0; i < argc; i++) {
			free(argv[i]);
		}
		
		int res = JackStart(server_name, driver_desc, driver_params, sync, temporary, client_timeout, realtime, realtime_priority, loopback, verbose_aux);
		if (res < 0) {
			jack_error("Cannot start server... exit");
			JackDelete();
			jack_cleanup_shm();
		#ifndef WIN32
			jack_cleanup_files(server_name);
		#endif
			jack_unregister_server(server_name);
			goto error;
		}
	}
	
	return true;
	
error:
	fClientCount--;
	return false;
}

void JackServerGlobals::Destroy()
{
    if (--fClientCount == 0) {
        JackLog("JackServerGlobals Destroy\n");
		JackStop();
		jack_cleanup_shm();
	#ifndef WIN32
		jack_cleanup_files(server_name);
	#endif
		jack_unregister_server(server_name);
    }
}

} // end of namespace


