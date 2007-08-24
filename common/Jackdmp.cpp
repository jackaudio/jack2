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
#include <signal.h>
#include <pwd.h>
#include <sys/types.h>
#include <dirent.h>
#include <getopt.h>

#include "JackServer.h"
#include "JackConstants.h"
#include "driver_interface.h"
#include "driver_parse.h"
#include "JackDriverLoader.h"
#include "jslist.h"
#include "JackError.h"
#include "shm.h"
#include "jack.h"

using namespace Jack;

static JackServer* fServer;
static char* server_name = NULL;
static int realtime_priority = 10;
static int do_mlock = 1;
static unsigned int port_max = 128;
static int realtime = 0;
static int loopback = 0;
static int temporary = 0;
static int client_timeout = 0; /* msecs; if zero, use period size. */
static int do_unlock = 0;
static JSList* drivers = NULL;

static sigset_t signals;

#define DEFAULT_TMP_DIR "/tmp"
char* jack_tmpdir = DEFAULT_TMP_DIR;

static void silent_jack_error_callback (const char *desc)
{}

static void copyright(FILE* file)
{
    fprintf (file, "jackdmp " VERSION "\n"
             "Copyright 2001-2005 Paul Davis and others.\n"
             "Copyright 2004-2007 Grame.\n"
             "jackdmp comes with ABSOLUTELY NO WARRANTY\n"
             "This is free software, and you are welcome to redistribute it\n"
             "under certain conditions; see the file COPYING for details\n");
}

static void usage (FILE* file)
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
             "               [ --silent OR -s ]\n"
             "               [ --sync OR -S ]\n"
             "               [ --version OR -V ]\n"
             "         -d driver [ ... driver args ... ]\n"
             "             where driver can be `alsa', `coreaudio', 'portaudio' or `dummy'\n"
             "       jackdmp -d driver --help\n"
             "             to display options for each driver\n\n");
}


static void DoNothingHandler(int sig)
{
    /* this is used by the child (active) process, but it never
       gets called unless we are already shutting down after
       another signal.
    */
    char buf[64];
    snprintf(buf, sizeof(buf), "received signal %d during shutdown (ignored)\n", sig);
    write(1, buf, strlen(buf));
}

static int JackStart(jack_driver_desc_t* driver_desc, JSList* driver_params, int sync, int temporary, int time_out_ms, int rt, int priority, int loopback, int verbose)
{
    JackLog("Jackdmp: sync = %ld timeout = %ld rt = %ld priority = %ld verbose = %ld \n", sync, time_out_ms, rt, priority, verbose);
    fServer = new JackServer(sync, temporary, time_out_ms, rt, priority, loopback, verbose);
    int res = fServer->Open(driver_desc, driver_params);
    return (res < 0) ? res : fServer->Start();
}

static int JackStop()
{
    fServer->Stop();
    fServer->Close();
    JackLog("Jackdmp: server close\n");
    delete fServer;
    JackLog("Jackdmp: delete server\n");
    return 0;
}

static int JackDelete()
{
    delete fServer;
    JackLog("Jackdmp: delete server\n");
    return 0;
}

static void FilterSIGPIPE()
{
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGPIPE);
    //sigprocmask(SIG_BLOCK, &set, 0);
    pthread_sigmask(SIG_BLOCK, &set, 0);
}

static char* jack_default_server_name(void)
{
    char *server_name;
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
        snprintf (user_dir, sizeof (user_dir), "%s/jack-%d",
                  jack_tmpdir, getuid ());
    }

    return user_dir;
}

/* returns the name of the per-server subdirectory of jack_user_dir() */

static char* get_jack_server_dir(const char* toto)
{
    static char server_dir[PATH_MAX] = "";

    // format the path name on the first call
    if (server_dir[0] == '\0') {
        snprintf (server_dir, sizeof (server_dir), "%s/%s",
                  jack_user_dir (), server_name);
    }

    return server_dir;
}

static void
jack_cleanup_files (const char *server_name)
{
    DIR *dir;
    struct dirent *dirent;
    char *dir_name = get_jack_server_dir (server_name);

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
    if ((dir = opendir (dir_name)) == NULL) {
        return ;
    }

    /* unlink all the files in this directory, they are mine */
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

    /* now, delete the per-server subdirectory, itself */
    if (rmdir (dir_name)) {
        jack_error ("cannot remove `%s' (%s)", dir_name,
                    strerror (errno));
    }

    /* finally, delete the per-user subdirectory, if empty */
    if (rmdir (jack_user_dir ())) {
        if (errno != ENOTEMPTY) {
            jack_error ("cannot remove `%s' (%s)",
                        jack_user_dir (), strerror (errno));
        }
    }
}

#ifdef FORK_SERVER

int main(int argc, char* argv[])
{
    int sig;
    sigset_t allsignals;
    struct sigaction action;
    int waiting;

    jack_driver_desc_t* driver_desc;
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
    int show_version = 0;
    int sync = 0;
    int rc, i;

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
                jack_verbose = 1;
                break;

            case 's':
                jack_set_error_function(silent_jack_error_callback);
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
                fprintf(stderr, "unknown option character %c\n",
                        optopt);
                /*fallthru*/
            case 'h':
                usage(stdout);
                return -1;
        }
    }

    /*
    if (show_version) {
    	printf ( "jackd version " VERSION 
    			" tmpdir " DEFAULT_TMP_DIR 
    			" protocol " PROTOCOL_VERSION
    			"\n");
    	return -1;
    }
    */

    if (!seen_driver) {
        usage (stderr);
        exit (1);
    }

    drivers = jack_drivers_load (drivers);
    if (!drivers) {
        fprintf (stderr, "jackdmp: no drivers found; exiting\n");
        exit (1);
    }

    driver_desc = jack_find_driver_descriptor (drivers, driver_name);
    if (!driver_desc) {
        fprintf (stderr, "jackdmp: unknown driver '%s'\n", driver_name);
        exit (1);
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
        exit (0);
    }

    if (server_name == NULL)
        server_name = jack_default_server_name ();

    copyright (stdout);

    rc = jack_register_server (server_name);
    switch (rc) {
        case EEXIST:
            fprintf (stderr, "`%s' server already active\n", server_name);
            exit (1);
        case ENOSPC:
            fprintf (stderr, "too many servers already active\n");
            exit (2);
        case ENOMEM:
            fprintf (stderr, "no access to shm registry\n");
            exit (3);
        default:
            if (jack_verbose)
                fprintf (stderr, "server `%s' registered\n",
                         server_name);
    }

    /* clean up shared memory and files from any previous
     * instance of this server name */
    jack_cleanup_shm();
    jack_cleanup_files(server_name);

    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    sigemptyset(&signals);
    sigaddset(&signals, SIGHUP);
    sigaddset(&signals, SIGINT);
    sigaddset(&signals, SIGQUIT);
    sigaddset(&signals, SIGPIPE);
    sigaddset(&signals, SIGTERM);
    sigaddset(&signals, SIGUSR1);
    sigaddset(&signals, SIGUSR2);

    // all child threads will inherit this mask unless they
    // explicitly reset it

    FilterSIGPIPE();
    pthread_sigmask(SIG_BLOCK, &signals, 0);

    if (!realtime && client_timeout == 0)
        client_timeout = 500; /* 0.5 sec; usable when non realtime. */

    int res = JackStart(driver_desc, driver_params, sync, temporary, client_timeout, realtime, realtime_priority, loopback, jack_verbose);
    if (res < 0) {
        jack_error("Cannot start server... exit");
        JackDelete();
        return 0;
    }

    /*
    For testing purpose...
    InternalMetro* client1  = new InternalMetro(1200, 0.4, 20, 80, "metro1");
    InternalMetro* client2  = new InternalMetro(600, 0.4, 20, 150, "metro2");
    InternalMetro* client3  = new InternalMetro(1000, 0.4, 20, 110, "metro3");
    InternalMetro* client4  = new InternalMetro(1200, 0.4, 20, 80, "metro4");
    InternalMetro* client5  = new InternalMetro(1500, 0.4, 20, 60, "metro5");
    InternalMetro* client6  = new InternalMetro(1200, 0.4, 20, 84, "metro6");
    InternalMetro* client7  = new InternalMetro(600, 0.4, 20, 160, "metro7");
    InternalMetro* client8  = new InternalMetro(1000, 0.4, 20, 113, "metro8");
    InternalMetro* client9  = new InternalMetro(1200, 0.4, 20, 84, "metro9");
    InternalMetro* client10  = new InternalMetro(1500, 0.4, 20, 70, "metro10");
    */

    // install a do-nothing handler because otherwise pthreads
    // behaviour is undefined when we enter sigwait.

    sigfillset(&allsignals);
    action.sa_handler = DoNothingHandler;
    action.sa_mask = allsignals;
    action.sa_flags = SA_RESTART | SA_RESETHAND;

    for (i = 1; i < NSIG; i++) {
        if (sigismember(&signals, i)) {
            sigaction(i, &action, 0);
        }
    }

    waiting = TRUE;

    while (waiting) {
        sigwait(&signals, &sig);

        fprintf(stderr, "jack main caught signal %d\n", sig);

        switch (sig) {
            case SIGUSR1:
                //jack_dump_configuration(engine, 1);
                break;
            case SIGUSR2:
                // driver exit
                waiting = FALSE;
                break;
            default:
                waiting = FALSE;
                break;
        }
    }

    if (sig != SIGSEGV) {
        // unblock signals so we can see them during shutdown.
        // this will help prod developers not to lose sight of
        // bugs that cause segfaults etc. during shutdown.
        sigprocmask(SIG_UNBLOCK, &signals, 0);
    }

    JackStop();

    jack_cleanup_shm();
    jack_cleanup_files(server_name);
    jack_unregister_server(server_name);

    return 1;
}

#else

int main(int argc, char* argv[])
{
    char c;
    long sample_sate = lopt(argv, "-r", 44100);
    long buffer_size = lopt(argv, "-p", 512);
    long chan_in = lopt(argv, "-i", 2);
    long chan_out = lopt(argv, "-o", 2);
    long audiodevice = lopt(argv, "-I", -1);
    long sync = lopt(argv, "-s", 0);
    long timeout = lopt(argv, "-t", 100 * 1000);
    const char* name = flag(argv, "-n", "Built-in Audio");
    long rt = lopt(argv, "-R", 0);
    verbose = lopt(argv, "-v", 0);

    copyright(stdout);
    usage(stdout);

    FilterSIGPIPE();

    printf("jackdmp: sample_sate = %ld buffer_size = %ld chan_in = %ld chan_out = %ld name = %s sync-mode = %ld\n",
           sample_sate, buffer_size, chan_in, chan_out, name, sync);
    assert(buffer_size <= BUFFER_SIZE_MAX);

    int res = JackStart(sample_sate, buffer_size, chan_in, chan_out, name, audiodevice, sync, timeout, rt);
    if (res < 0) {
        jack_error("Cannot start server... exit");
        JackDelete();
        return 0;
    }

    while (((c = getchar()) != 'q')) {

        switch (c) {

            case 's':
                fServer->PrintState();
                break;
        }
    }

    JackStop();
    return 0;
}

#endif
