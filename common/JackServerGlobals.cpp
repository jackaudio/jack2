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

#include "JackServerGlobals.h"
#include "JackLockedEngine.h"
#include "JackTools.h"
#include "shm.h"
#include <getopt.h>
#include <errno.h>

static char* server_name = NULL;

namespace Jack
{

JackServer* JackServerGlobals::fInstance;
unsigned int JackServerGlobals::fUserCount;
int JackServerGlobals::fRTNotificationSocket;
std::map<std::string, JackDriverInfo*> JackServerGlobals::fSlavesList;
std::map<std::string, int> JackServerGlobals::fInternalsList;

bool (* JackServerGlobals::on_device_acquire)(const char * device_name) = NULL;
void (* JackServerGlobals::on_device_release)(const char * device_name) = NULL;

int JackServerGlobals::Start(const char* server_name,
                             jack_driver_desc_t* driver_desc,
                             JSList* driver_params,
                             int sync,
                             int temporary,
                             int time_out_ms,
                             int rt,
                             int priority,
                             int port_max,
                             int verbose,
                            jack_timer_type_t clock)
{
    jack_log("Jackdmp: sync = %ld timeout = %ld rt = %ld priority = %ld verbose = %ld ", sync, time_out_ms, rt, priority, verbose);
    new JackServer(sync, temporary, time_out_ms, rt, priority, port_max, verbose, clock, server_name);  // Will setup fInstance and fUserCount globals
    int res = fInstance->Open(driver_desc, driver_params);
    return (res < 0) ? res : fInstance->Start();
}

void JackServerGlobals::Stop()
{
    jack_log("Jackdmp: server close");
    fInstance->Stop();
    fInstance->Close();
}

void JackServerGlobals::Delete()
{
    jack_log("Jackdmp: delete server");

    // Slave drivers
    std::map<std::string, JackDriverInfo*>::iterator it1;
    for (it1 = fSlavesList.begin(); it1 != fSlavesList.end(); it1++) {
        JackDriverInfo* info = (*it1).second;
        if (info) {
            fInstance->RemoveSlave((info));
            delete (info);
        }
    }
    fSlavesList.clear();

    // Internal clients
    std::map<std::string, int> ::iterator it2;
    for (it2 = fInternalsList.begin(); it2 != fInternalsList.end(); it2++) {
        int status;
        int refnum = (*it2).second;
        if (refnum > 0) {
            // Client object is internally kept in JackEngine, and will be deallocated in InternalClientUnload
            fInstance->GetEngine()->InternalClientUnload(refnum, &status);
        }
    }
    fInternalsList.clear();

    delete fInstance;
    fInstance = NULL;
}

bool JackServerGlobals::Init()
{
    int realtime = 0;
    int client_timeout = 0; /* msecs; if zero, use period size. */
    int realtime_priority = 10;
    int verbose_aux = 0;
    unsigned int port_max = 128;
    int temporary = 0;

    int opt = 0;
    int option_index = 0;
    char *master_driver_name = NULL;
    char **master_driver_args = NULL;
    JSList* master_driver_params = NULL;
    jack_driver_desc_t* driver_desc;
    jack_timer_type_t clock_source = JACK_TIMER_SYSTEM_CLOCK;
    int driver_nargs = 1;
    JSList* drivers = NULL;
    int loopback = 0;
    int sync = 0;
    int rc, i;
    int ret;
    int replace_registry = 0;

    FILE* fp = 0;
    char filename[255];
    char buffer[255];
    int argc = 0;
    char* argv[32];

    // First user starts the server
    if (fUserCount++ == 0) {

        jack_log("JackServerGlobals Init");

        const char *options = "-d:X:I:P:uvshVrRL:STFl:t:mn:p:"
    #ifdef __linux__
            "c:"
    #endif
        ;

    struct option long_options[] = {
    #ifdef __linux__
                                       { "clock-source", 1, 0, 'c' },
    #endif
                                       { "loopback-driver", 1, 0, 'L' },
                                       { "audio-driver", 1, 0, 'd' },
                                       { "midi-driver", 1, 0, 'X' },
                                       { "internal-client", 1, 0, 'I' },
                                       { "verbose", 0, 0, 'v' },
                                       { "help", 0, 0, 'h' },
                                       { "port-max", 1, 0, 'p' },
                                       { "no-mlock", 0, 0, 'm' },
                                       { "name", 1, 0, 'n' },
                                       { "unlock", 0, 0, 'u' },
                                       { "realtime", 0, 0, 'R' },
                                       { "no-realtime", 0, 0, 'r' },
                                       { "replace-registry", 0, &replace_registry, 0 },
                                       { "loopback", 0, 0, 'L' },
                                       { "realtime-priority", 1, 0, 'P' },
                                       { "timeout", 1, 0, 't' },
                                       { "temporary", 0, 0, 'T' },
                                       { "version", 0, 0, 'V' },
                                       { "silent", 0, 0, 's' },
                                       { "sync", 0, 0, 'S' },
                                       { 0, 0, 0, 0 }
                                   };

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

        while (!master_driver_name &&
                (opt = getopt_long(argc, argv, options, long_options, &option_index)) != EOF) {

            switch (opt) {

                case 'c':
                    if (tolower (optarg[0]) == 'h') {
                        clock_source = JACK_TIMER_HPET;
                    } else if (tolower (optarg[0]) == 'c') {
                        clock_source = JACK_TIMER_CYCLE_COUNTER;
                    } else if (tolower (optarg[0]) == 's') {
                        clock_source = JACK_TIMER_SYSTEM_CLOCK;
                    } else {
                        jack_error("unknown option character %c", optopt);
                    }
                    break;

                case 'd':
                    master_driver_name = optarg;
                    break;

                case 'L':
                    loopback = atoi(optarg);
                    break;

                 case 'X':
                    fSlavesList[optarg] = NULL;
                    break;

                case 'I':
                    fInternalsList[optarg] = -1;
                    break;

                case 'p':
                    port_max = (unsigned int)atol(optarg);
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

                case 'P':
                    realtime_priority = atoi(optarg);
                    break;

                case 'r':
                    realtime = 0;
                    break;

                case 'R':
                    realtime = 1;
                    break;

                case 'T':
                    temporary = 1;
                    break;

                case 't':
                    client_timeout = atoi(optarg);
                    break;

                default:
                    jack_error("unknown option character %c", optopt);
                    break;
            }
        }

        drivers = jack_drivers_load(drivers);
        if (!drivers) {
            jack_error("jackdmp: no drivers found; exiting");
            goto error;
        }

        driver_desc = jack_find_driver_descriptor(drivers, master_driver_name);
        if (!driver_desc) {
            jack_error("jackdmp: unknown master driver '%s'", master_driver_name);
            goto error;
        }

        if (optind < argc) {
            driver_nargs = 1 + argc - optind;
        } else {
            driver_nargs = 1;
        }

        if (driver_nargs == 0) {
            jack_error("No driver specified ... hmm. JACK won't do"
                       " anything when run like this.");
            goto error;
        }

        master_driver_args = (char**)malloc(sizeof(char*) * driver_nargs);
        master_driver_args[0] = master_driver_name;

        for (i = 1; i < driver_nargs; i++) {
            master_driver_args[i] = argv[optind++];
        }

        if (jack_parse_driver_params(driver_desc, driver_nargs, master_driver_args, &master_driver_params)) {
            goto error;
        }

#ifndef WIN32
        if (server_name == NULL)
            server_name = (char*)JackTools::DefaultServerName();
#endif

        rc = jack_register_server(server_name, false);
        switch (rc) {
            case EEXIST:
                jack_error("`%s' server already active", server_name);
                goto error;
            case ENOSPC:
                jack_error("too many servers already active");
                goto error;
            case ENOMEM:
                jack_error("no access to shm registry");
                goto error;
            default:
                jack_info("server `%s' registered", server_name);
        }

        /* clean up shared memory and files from any previous instance of this server name */
        jack_cleanup_shm();
        JackTools::CleanupFiles(server_name);

        if (!realtime && client_timeout == 0)
            client_timeout = 500; /* 0.5 sec; usable when non realtime. */

        for (i = 0; i < argc; i++) {
            free(argv[i]);
        }

        int res = Start(server_name, driver_desc, master_driver_params, sync, temporary, client_timeout, realtime, realtime_priority, port_max, verbose_aux, clock_source);
        if (res < 0) {
            jack_error("Cannot start server... exit");
            Delete();
            jack_cleanup_shm();
            JackTools::CleanupFiles(server_name);
            jack_unregister_server(server_name);
            goto error;
        }

        // Slave drivers
        std::map<std::string, JackDriverInfo*>::iterator it1;
        for (it1 = fSlavesList.begin(); it1 != fSlavesList.end(); it1++) {
            const char* name = ((*it1).first).c_str();
            driver_desc = jack_find_driver_descriptor(drivers, name);
            if (!driver_desc) {
                jack_error("jackdmp: unknown slave driver '%s'", name);
            } else {
                (*it1).second = fInstance->AddSlave(driver_desc, NULL);
            }
        }

        // Loopback driver
        if (loopback > 0) {
            driver_desc = jack_find_driver_descriptor(drivers, "loopback");
            if (!driver_desc) {
                jack_error("jackdmp: unknown driver '%s'", "loopback");
            } else {
                fSlavesList["loopback"] = fInstance->AddSlave(driver_desc, NULL);
            }
        }

        // Internal clients
        std::map<std::string, int>::iterator it2;
        for (it2 = fInternalsList.begin(); it2 != fInternalsList.end(); it2++) {
            int status, refnum;
            const char* name = ((*it2).first).c_str();
            fInstance->InternalClientLoad2(name, name, NULL, JackNullOption, &refnum, -1, &status);
            (*it2).second = refnum;
        }
    }

    if (master_driver_params)
        jack_free_driver_params(master_driver_params);
    return true;

error:
    jack_log("JackServerGlobals Init error");
    if (master_driver_params)
        jack_free_driver_params(master_driver_params);
    Destroy();
    return false;
}

void JackServerGlobals::Destroy()
{
    if (--fUserCount == 0) {
        jack_log("JackServerGlobals Destroy");
        Stop();
        Delete();
        jack_cleanup_shm();
        JackTools::CleanupFiles(server_name);
        jack_unregister_server(server_name);
    }
}

} // end of namespace


