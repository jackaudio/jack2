/*
Copyright (C) 2001 Paul Davis 
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

#include <iostream>
#include <assert.h>
#include <process.h>
#include <getopt.h>
#include <signal.h>

#include "JackServer.h"
#include "JackConstants.h" 
#include "driver_interface.h"
#include "JackDriverLoader.h"
#include "shm.h"

using namespace Jack;

static JackServer* fServer;
static char *server_name = "default";
static int realtime_priority = 10;
static int do_mlock = 1;
static unsigned int port_max = 128;
static int realtime = 0;
static int loopback = 0;
static int temporary = 0;
static int client_timeout = 0; /* msecs; if zero, use period size. */
static int do_unlock = 0;
static JSList* drivers = NULL;
static int sync = 0;
static int xverbose = 0;

#define DEFAULT_TMP_DIR "/tmp"
char *jack_tmpdir = DEFAULT_TMP_DIR;

HANDLE waitEvent;

void jack_print_driver_options (jack_driver_desc_t * desc, FILE *file);
void jack_print_driver_param_usage (jack_driver_desc_t * desc, unsigned long param, FILE *file);
int jack_parse_driver_params (jack_driver_desc_t * desc, int argc, char* argv[], JSList ** param_ptr);
static void silent_jack_error_callback (const char *desc)
{}

static void copyright(FILE* file)
{
    fprintf (file, "jackdmp " VERSION "\n"
             "Copyright 2001-2005 Paul Davis and others.\n"
             "Copyright 2004-2008 Grame.\n"
             "jackdmp comes with ABSOLUTELY NO WARRANTY\n"
             "This is free software, and you are welcome to redistribute it\n"
             "under certain conditions; see the file COPYING for details\n");
}

static void usage (FILE *file)
{
    copyright (file);
    fprintf (file, "\n"
             "usage: jackdmp [ --realtime OR -R [ --realtime-priority OR -P priority ] ]\n"
             "               [ --name OR -n server-name ]\n"
             // "               [ --no-mlock OR -m ]\n"
             // "               [ --unlock OR -u ]\n"
             "               [ --timeout OR -t client-timeout-in-msecs ]\n"
             "               [ --loopback OR -L loopback-port-number ]\n"
             // "               [ --port-max OR -p maximum-number-of-ports]\n"
             "               [ --verbose OR -v ]\n"
			 "               [ --replace-registry OR -r ]\n"
             "               [ --silent OR -s ]\n"
             "               [ --sync OR -S ]\n"
             "               [ --version OR -V ]\n"
             "         -d driver [ ... driver args ... ]\n"
             "             where driver can be `alsa', `coreaudio', 'portaudio' or `dummy'\n"
             "       jackdmp -d driver --help\n"
             "             to display options for each driver\n\n");
}

static int JackStart(jack_driver_desc_t* driver_desc, JSList* driver_params, int sync, int time_out_ms, int rt, int priority, int loopback, int verbose, const char* server_name)
{
    printf("Jackdmp: sync = %ld timeout = %ld rt = %ld priority = %ld verbose = %ld \n", sync, time_out_ms, rt, priority, verbose);
    fServer = new JackServer(sync, temporary, time_out_ms, rt, priority, loopback, verbose, server_name);
    int res = fServer->Open(driver_desc, driver_params);
    return (res < 0) ? res : fServer->Start();
}

static int JackStop()
{
    fServer->Stop();
    fServer->Close();
    printf("Jackdmp: server close\n");
    delete fServer;
    printf("Jackdmp: delete server\n");
    return 0;
}

static int JackDelete()
{
    delete fServer;
    printf("Jackdmp: delete server\n");
    return 0;
}

static void intrpt(int signum)
{
    printf("jack main caught signal %d\n", signum);
    (void) signal(SIGINT, SIG_DFL);
	SetEvent(waitEvent);
}

/*
static char* jack_default_server_name(void)
{
	char *server_name;
	if ((server_name = getenv("JACK_DEFAULT_SERVER")) == NULL)
		server_name = "default";
	return server_name;
}
 
// returns the name of the per-user subdirectory of jack_tmpdir 
static char* jack_user_dir(void)
{
	static char user_dir[PATH_MAX] = "";
 
	// format the path name on the first call 
	if (user_dir[0] == '\0') {
		snprintf (user_dir, sizeof (user_dir), "%s/jack-%d",
			  jack_tmpdir, _getuid ());
	}
 
	return user_dir;
}
 
// returns the name of the per-server subdirectory of jack_user_dir() 
 
static char* get_jack_server_dir(const char * toto)
{
	static char server_dir[PATH_MAX] = "";
 
	// format the path name on the first call 
	if (server_dir[0] == '\0') {
		snprintf (server_dir, sizeof (server_dir), "%s/%s",
			  jack_user_dir (), server_name);
	}
 
	return server_dir;
}
 
static void jack_cleanup_files (const char *server_name)
{
	DIR *dir;
	struct dirent *dirent;
	char *dir_name = get_jack_server_dir (server_name);
 
	// nothing to do if the server directory does not exist 
	if ((dir = opendir (dir_name)) == NULL) {
		return;
	}
 
	// unlink all the files in this directory, they are mine 
	while ((dirent = readdir (dir)) != NULL) {
 
		char fullpath[PATH_MAX];
 
		if ((strcmp (dirent->d_name, ".") == 0)
		    || (strcmp (dirent->d_name, "..") == 0)) {
			continue;
		}
 
		snprintf (fullpath, sizeof (fullpath), "%s/%s",
			  dir_name, dirent->d_name);
 
		if (unlink (fullpath)) {
			jack_error ("cannot unlink `%s' (%s)", fullpath,
				    strerror (errno));
		}
	} 
 
	closedir (dir);
 
	// now, delete the per-server subdirectory, itself 
	if (rmdir (dir_name)) {
 		jack_error ("cannot remove `%s' (%s)", dir_name,
			    strerror (errno));
	}
 
	// finally, delete the per-user subdirectory, if empty 
	if (rmdir (jack_user_dir ())) {
		if (errno != ENOTEMPTY) {
			jack_error ("cannot remove `%s' (%s)",
				    jack_user_dir (), strerror (errno));
		}
	}
}
*/

/*
BOOL CtrlHandler( DWORD fdwCtrlType ) 
{ 
  switch( fdwCtrlType ) 
  { 
    // Handle the CTRL-C signal. 
    case CTRL_C_EVENT: 
      printf( "Ctrl-C event\n\n" );
      Beep( 750, 300 ); 
	  SetEvent(waitEvent);
      return( TRUE );
 
    // CTRL-CLOSE: confirm that the user wants to exit. 
    case CTRL_CLOSE_EVENT: 
      Beep( 600, 200 ); 
      printf( "Ctrl-Close event\n\n" );
	  SetEvent(waitEvent);
      return( TRUE ); 
 
    // Pass other signals to the next handler. 
    case CTRL_BREAK_EVENT: 
      Beep( 900, 200 ); 
      printf( "Ctrl-Break event\n\n" );
      return FALSE; 
 
    case CTRL_LOGOFF_EVENT: 
      Beep( 1000, 200 ); 
      printf( "Ctrl-Logoff event\n\n" );
      return FALSE; 
 
    case CTRL_SHUTDOWN_EVENT: 
      Beep( 750, 500 ); 
      printf( "Ctrl-Shutdown event\n\n" );
      return FALSE; 
 
    default: 
      return FALSE; 
  } 
} 
 
*/

int main(int argc, char* argv[])
{
    jack_driver_desc_t * driver_desc;
    const char *options = "-ad:P:uvshVRL:STFl:t:mn:p:";
    struct option long_options[] = {
                                       { "driver", 1, 0, 'd'
                                       },
                                       { "verbose", 0, 0, 'v' },
                                       { "help", 0, 0, 'h' },
                                       { "port-max", 1, 0, 'p' },
                                       { "no-mlock", 0, 0, 'm' },
                                       { "name", 0, 0, 'n' },
                                       { "unlock", 0, 0, 'u' },
                                       { "realtime", 0, 0, 'R' },
									   { "replace-registry", 0, 0, 'r' },
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
    JSList * driver_params;
    int driver_nargs = 1;
    int show_version = 0;
	int replace_registry = 0;
    int sync = 0;
    int i;
    int rc;
    char c;

	// Creates wait event
	if ((waitEvent = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL) {
        printf("CreateEvent fails err = %ld\n", GetLastError());
        return 0;
    }

    opterr = 0;
    while (!seen_driver &&
            (opt = getopt_long(argc, argv, options,
                               long_options, &option_index)) != EOF) {
        switch (opt) {

            case 'd':
                seen_driver = 1;
                driver_name = optarg;
                break;

            case 'v':
                xverbose = 1;
                break;

            case 's':
                // steph
                //jack_set_error_function(silent_jack_error_callback);
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
				
			case 'r':
				replace_registry = 1;
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
                fprintf(stderr, "unknown option character %c\n",
                        optopt);
                /*fallthru*/
            case 'h':
                usage(stdout);
                return -1;
        }
    }

    if (!seen_driver) {
        usage (stderr);
        //exit (1);
		return 0;
    }

    drivers = jack_drivers_load (drivers);
    if (!drivers) {
        fprintf (stderr, "jackdmp: no drivers found; exiting\n");
        //exit (1);
		return 0;
    }

    driver_desc = jack_find_driver_descriptor (drivers, driver_name);
    if (!driver_desc) {
        fprintf (stderr, "jackdmp: unknown driver '%s'\n", driver_name);
        //exit (1);
		return 0;
    }

    if (optind < argc) {
        driver_nargs = 1 + argc - optind;
    } else {
        driver_nargs = 1;
    }

    if (driver_nargs == 0) {
        fprintf (stderr, "No driver specified ... hmm. JACK won't do"
                 " anything when run like this.\n");
        return -1;
    }

    driver_args = (char **) malloc (sizeof (char *) * driver_nargs);
    driver_args[0] = driver_name;

    for (i = 1; i < driver_nargs; i++) {
        driver_args[i] = argv[optind++];
    }

    if (jack_parse_driver_params (driver_desc, driver_nargs,
                                  driver_args, &driver_params)) {
       // exit (0);
		return 0;
    }

    //if (server_name == NULL)
    //	server_name = jack_default_server_name ();

    copyright (stdout);

    rc = jack_register_server (server_name, replace_registry);
    switch (rc) {
        case EEXIST:
            fprintf (stderr, "`%s' server already active\n", server_name);
            //exit (1);
			return 0;
        case ENOSPC:
            fprintf (stderr, "too many servers already active\n");
            //exit (2);
			return 0;
        case ENOMEM:
            fprintf (stderr, "no access to shm registry\n");
            //exit (3);
			return 0;
        default:
            if (xverbose)
                fprintf (stderr, "server `%s' registered\n",
                         server_name);
    }


    /* clean up shared memory and files from any previous
     * instance of this server name */
    jack_cleanup_shm();
    //	jack_cleanup_files(server_name);

    if (!realtime && client_timeout == 0)
        client_timeout = 500; /* 0.5 sec; usable when non realtime. */

    int res = JackStart(driver_desc, driver_params, sync, client_timeout, realtime, realtime_priority, loopback, xverbose, server_name);
    if (res < 0) {
        printf("Cannot start server... exit\n");
        JackDelete();
        return 0;
    }

	/*
	if( SetConsoleCtrlHandler( (PHANDLER_ROUTINE) CtrlHandler, TRUE ) ) 
	{ 
		printf( "\nThe Control Handler is installed.\n" ); 
	} else {
		printf( "\nERROR: Could not set control handler"); 
	}
	*/
	
	
	(void) signal(SIGINT, intrpt);
	(void) signal(SIGABRT, intrpt);
	(void) signal(SIGTERM, intrpt);

	if ((res = WaitForSingleObject(waitEvent, INFINITE)) != WAIT_OBJECT_0) {
        printf("WaitForSingleObject fails err = %ld\n", GetLastError());
    }
	
	/*
    printf("Type 'q' to quit\n");
    while ((c = getchar()) != 'q') {}
	*/
	

    JackStop();

    jack_cleanup_shm();
    //	jack_cleanup_files(server_name);
    jack_unregister_server(server_name);

	CloseHandle(waitEvent);
    return 1;
}


