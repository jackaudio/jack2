/*
Copyright (C) 2001-2005 Paul Davis
Copyright (C) 2004-2008 Grame

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

#include "JackSystemDeps.h"
#include "JackDriverLoader.h"
#include "JackConstants.h"
#include "JackError.h"
#include <getopt.h>
#include <stdio.h>
#include <errno.h>

#ifndef WIN32
#include <dirent.h>
#endif

jack_driver_desc_t * jackctl_driver_get_desc(jackctl_driver_t * driver);

EXPORT void jack_print_driver_options (jack_driver_desc_t* desc, FILE* file)
{
    unsigned long i;
    char arg_default[JACK_DRIVER_PARAM_STRING_MAX + 1];

    for (i = 0; i < desc->nparams; i++) {
        switch (desc->params[i].type) {
            case JackDriverParamInt:
                sprintf (arg_default, "%" "i", desc->params[i].value.i);
                break;
            case JackDriverParamUInt:
                sprintf (arg_default, "%" "u", desc->params[i].value.ui);
                break;
            case JackDriverParamChar:
                sprintf (arg_default, "%c", desc->params[i].value.c);
                break;
            case JackDriverParamString:
                if (desc->params[i].value.str && strcmp (desc->params[i].value.str, "") != 0)
                    sprintf (arg_default, "%s", desc->params[i].value.str);
                else
                    sprintf (arg_default, "none");
                break;
            case JackDriverParamBool:
                sprintf (arg_default, "%s", desc->params[i].value.i ? "true" : "false");
                break;
        }

        fprintf (file, "\t-%c, --%s \t%s (default: %s)\n",
                 desc->params[i].character,
                 desc->params[i].name,
                 desc->params[i].long_desc,
                 arg_default);
    }
}

static void
jack_print_driver_param_usage (jack_driver_desc_t * desc, unsigned long param, FILE *file)
{
    fprintf (file, "Usage information for the '%s' parameter for driver '%s':\n",
             desc->params[param].name, desc->name);
    fprintf (file, "%s\n", desc->params[param].long_desc);
}

EXPORT void jack_free_driver_params(JSList * driver_params)
{
    JSList *node_ptr = driver_params;
    JSList *next_node_ptr;

    while (node_ptr) {
        next_node_ptr = node_ptr->next;
        free(node_ptr->data);
        free(node_ptr);
        node_ptr = next_node_ptr;
    }
}

int
jack_parse_driver_params (jack_driver_desc_t * desc, int argc, char* argv[], JSList ** param_ptr)
{
    struct option * long_options;
    char * options, * options_ptr;
    unsigned long i;
    int opt;
    unsigned int param_index;
    JSList * params = NULL;
    jack_driver_param_t * driver_param;

    if (argc <= 1) {
        *param_ptr = NULL;
        return 0;
    }

    /* check for help */
    if (strcmp (argv[1], "-h") == 0 || strcmp (argv[1], "--help") == 0) {
        if (argc > 2) {
            for (i = 0; i < desc->nparams; i++) {
                if (strcmp (desc->params[i].name, argv[2]) == 0) {
                    jack_print_driver_param_usage (desc, i, stdout);
                    return 1;
                }
            }

            fprintf (stderr, "jackd: unknown option '%s' "
                     "for driver '%s'\n", argv[2],
                     desc->name);
        }

        jack_log("Parameters for driver '%s' (all parameters are optional):", desc->name);
        jack_print_driver_options (desc, stdout);
        return 1;
    }

    /* set up the stuff for getopt */
    options = (char*)calloc (desc->nparams * 3 + 1, sizeof (char));
    long_options = (option*)calloc (desc->nparams + 1, sizeof (struct option));

    options_ptr = options;
    for (i = 0; i < desc->nparams; i++) {
        sprintf (options_ptr, "%c::", desc->params[i].character);
        options_ptr += 3;
        long_options[i].name = desc->params[i].name;
        long_options[i].flag = NULL;
        long_options[i].val = desc->params[i].character;
        long_options[i].has_arg = optional_argument;
    }

    /* create the params */
    optind = 0;
    opterr = 0;
    while ((opt = getopt_long(argc, argv, options, long_options, NULL)) != -1) {

        if (opt == ':' || opt == '?') {
            if (opt == ':') {
                fprintf (stderr, "Missing option to argument '%c'\n", optopt);
            } else {
                fprintf (stderr, "Unknownage with option '%c'\n", optopt);
            }

            fprintf (stderr, "Options for driver '%s':\n", desc->name);
            jack_print_driver_options (desc, stderr);
            return 1;
        }

        for (param_index = 0; param_index < desc->nparams; param_index++) {
            if (opt == desc->params[param_index].character) {
                break;
            }
        }

        driver_param = (jack_driver_param_t*)calloc (1, sizeof (jack_driver_param_t));
        driver_param->character = desc->params[param_index].character;

        if (!optarg && optind < argc &&
                strlen(argv[optind]) &&
                argv[optind][0] != '-') {
            optarg = argv[optind];
        }

        if (optarg) {
            switch (desc->params[param_index].type) {
                case JackDriverParamInt:
                    driver_param->value.i = atoi (optarg);
                    break;
                case JackDriverParamUInt:
                    driver_param->value.ui = strtoul (optarg, NULL, 10);
                    break;
                case JackDriverParamChar:
                    driver_param->value.c = optarg[0];
                    break;
                case JackDriverParamString:
                    strncpy (driver_param->value.str, optarg, JACK_DRIVER_PARAM_STRING_MAX);
                    break;
                case JackDriverParamBool:

                    /*
                                if (strcasecmp ("false", optarg) == 0 ||
                                        strcasecmp ("off", optarg) == 0 ||
                                        strcasecmp ("no", optarg) == 0 ||
                                        strcasecmp ("0", optarg) == 0 ||
                                        strcasecmp ("(null)", optarg) == 0 ) {
                    */
                    // steph
                    if (strcmp ("false", optarg) == 0 ||
                            strcmp ("off", optarg) == 0 ||
                            strcmp ("no", optarg) == 0 ||
                            strcmp ("0", optarg) == 0 ||
                            strcmp ("(null)", optarg) == 0 ) {
                        driver_param->value.i = false;

                    } else {

                        driver_param->value.i = true;

                    }
                    break;
            }
        } else {
            if (desc->params[param_index].type == JackDriverParamBool) {
                driver_param->value.i = true;
            } else {
                driver_param->value = desc->params[param_index].value;
            }
        }

        params = jack_slist_append (params, driver_param);
    }

    free (options);
    free (long_options);

    if (param_ptr)
        *param_ptr = params;

    return 0;
}

EXPORT int
jackctl_parse_driver_params (jackctl_driver *driver_ptr, int argc, char* argv[])
{
    struct option * long_options;
    char * options, * options_ptr;
    unsigned long i;
    int opt;
    JSList * node_ptr;
    jackctl_parameter_t * param = NULL;
    union jackctl_parameter_value value;

    if (argc <= 1)
        return 0;

    const JSList * driver_params = jackctl_driver_get_parameters(driver_ptr);
    if (driver_params == NULL)
        return 1;

    jack_driver_desc_t * desc = jackctl_driver_get_desc(driver_ptr);

    /* check for help */
    if (strcmp (argv[1], "-h") == 0 || strcmp (argv[1], "--help") == 0) {
        if (argc > 2) {
            for (i = 0; i < desc->nparams; i++) {
                if (strcmp (desc->params[i].name, argv[2]) == 0) {
                    jack_print_driver_param_usage (desc, i, stdout);
                    return 1;
                }
            }

            fprintf (stderr, "jackd: unknown option '%s' "
                     "for driver '%s'\n", argv[2],
                     desc->name);
        }

        jack_log("Parameters for driver '%s' (all parameters are optional):", desc->name);
        jack_print_driver_options (desc, stdout);
        return 1;
    }

   /* set up the stuff for getopt */
    options = (char*)calloc (desc->nparams * 3 + 1, sizeof (char));
    long_options = (option*)calloc (desc->nparams + 1, sizeof (struct option));

    options_ptr = options;
    for (i = 0; i < desc->nparams; i++) {
        sprintf (options_ptr, "%c::", desc->params[i].character);
        options_ptr += 3;
        long_options[i].name = desc->params[i].name;
        long_options[i].flag = NULL;
        long_options[i].val = desc->params[i].character;
        long_options[i].has_arg = optional_argument;
    }

    /* create the params */
    optind = 0;
    opterr = 0;
    while ((opt = getopt_long(argc, argv, options, long_options, NULL)) != -1) {

        if (opt == ':' || opt == '?') {
            if (opt == ':') {
                fprintf (stderr, "Missing option to argument '%c'\n", optopt);
            } else {
                fprintf (stderr, "Unknownage with option '%c'\n", optopt);
            }

            fprintf (stderr, "Options for driver '%s':\n", desc->name);
            jack_print_driver_options(desc, stderr);
            return 1;
        }

        node_ptr = (JSList *)driver_params;
       	while (node_ptr) {
            param = (jackctl_parameter_t*)node_ptr->data;
            if (opt == jackctl_parameter_get_id(param)) {
                break;
            }
            node_ptr = node_ptr->next;
        }

        if (!optarg && optind < argc &&
            strlen(argv[optind]) &&
            argv[optind][0] != '-') {
            optarg = argv[optind];
        }

        if (optarg) {
            switch (jackctl_parameter_get_type(param)) {
                case JackDriverParamInt:
                    value.i = atoi (optarg);
                    jackctl_parameter_set_value(param, &value);
                    break;
                case JackDriverParamUInt:
                    value.ui = strtoul (optarg, NULL, 10);
                    jackctl_parameter_set_value(param, &value);
                    break;
                case JackDriverParamChar:
                    value.c = optarg[0];
                    jackctl_parameter_set_value(param, &value);
                    break;
                case JackDriverParamString:
                    strncpy (value.str, optarg, JACK_DRIVER_PARAM_STRING_MAX);
                    jackctl_parameter_set_value(param, &value);
                    break;
                case JackDriverParamBool:
                    /*
                     if (strcasecmp ("false", optarg) == 0 ||
                         strcasecmp ("off", optarg) == 0 ||
                         strcasecmp ("no", optarg) == 0 ||
                         strcasecmp ("0", optarg) == 0 ||
                         strcasecmp ("(null)", optarg) == 0 ) {
                    */
                    // steph
                    if (strcmp ("false", optarg) == 0 ||
                        strcmp ("off", optarg) == 0 ||
                        strcmp ("no", optarg) == 0 ||
                        strcmp ("0", optarg) == 0 ||
                        strcmp ("(null)", optarg) == 0 ) {
                        value.i = false;
                    } else {
                        value.i = true;
                    }
                    jackctl_parameter_set_value(param, &value);
                    break;
            }
        } else {
            if (jackctl_parameter_get_type(param) == JackParamBool) {
                value.i = true;
            } else {
                value = jackctl_parameter_get_default_value(param);
            }
            jackctl_parameter_set_value(param, &value);
        }
    }

    free(options);
    free(long_options);
    return 0;
}

jack_driver_desc_t *
jack_find_driver_descriptor (JSList * drivers, const char * name)
{
    jack_driver_desc_t * desc = 0;
    JSList * node;

    for (node = drivers; node; node = jack_slist_next (node)) {
        desc = (jack_driver_desc_t *) node->data;

        if (strcmp (desc->name, name) != 0) {
            desc = NULL;
        } else {
            break;
        }
    }

    return desc;
}

static jack_driver_desc_t *
jack_get_descriptor (JSList * drivers, const char * sofile, const char * symbol)
{
    jack_driver_desc_t * descriptor, * other_descriptor;
    JackDriverDescFunction so_get_descriptor = NULL;
    JSList * node;
    void * dlhandle;
    char * filename;
#ifdef WIN32
    int dlerr;
#else
    const char * dlerr;
#endif

    int err;
    const char* driver_dir;

    if ((driver_dir = getenv("JACK_DRIVER_DIR")) == 0) {
        // for WIN32 ADDON_DIR is defined in JackConstants.h as relative path
        // for posix systems, it is absolute path of default driver dir
#ifdef WIN32
        char temp_driver_dir1[512];
        char temp_driver_dir2[512];
        GetCurrentDirectory(512, temp_driver_dir1);
        sprintf(temp_driver_dir2, "%s/%s", temp_driver_dir1, ADDON_DIR);
        driver_dir = temp_driver_dir2;
#else
        driver_dir = ADDON_DIR;
#endif
    }

    filename = (char *)malloc(strlen (driver_dir) + 1 + strlen(sofile) + 1);
    sprintf (filename, "%s/%s", driver_dir, sofile);

    if ((dlhandle = LoadDriverModule(filename)) == NULL) {
#ifdef WIN32
        jack_error ("could not open driver .dll '%s': %ld", filename, GetLastError());
#else
        jack_error ("could not open driver .so '%s': %s", filename, dlerror());
#endif

        free(filename);
        return NULL;
    }

    so_get_descriptor = (JackDriverDescFunction)GetDriverProc(dlhandle, symbol);

#ifdef WIN32
    if ((so_get_descriptor == NULL) && (dlerr = GetLastError()) != 0) {
        jack_error("jack_get_descriptor : dll is not a driver, err = %ld", dlerr);
#else
    if ((so_get_descriptor == NULL) && (dlerr = dlerror ()) != NULL) {
        jack_error("jack_get_descriptor err = %s", dlerr);
#endif

        UnloadDriverModule(dlhandle);
        free(filename);
        return NULL;
    }

    if ((descriptor = so_get_descriptor ()) == NULL) {
        jack_error("driver from '%s' returned NULL descriptor", filename);
        UnloadDriverModule(dlhandle);
        free(filename);
        return NULL;
    }

#ifdef WIN32
    if ((err = UnloadDriverModule(dlhandle)) == 0) {
        jack_error ("error closing driver .so '%s': %ld", filename, GetLastError ());
    }
#else
    if ((err = UnloadDriverModule(dlhandle)) != 0) {
        jack_error ("error closing driver .so '%s': %s", filename, dlerror ());
    }
#endif

    /* check it doesn't exist already */
    for (node = drivers; node; node = jack_slist_next (node)) {
        other_descriptor = (jack_driver_desc_t *) node->data;

        if (strcmp(descriptor->name, other_descriptor->name) == 0) {
            jack_error("the drivers in '%s' and '%s' both have the name '%s'; using the first",
                       other_descriptor->file, filename, other_descriptor->name);
            /* FIXME: delete the descriptor */
            free(filename);
            return NULL;
        }
    }

    strncpy(descriptor->file, filename, JACK_PATH_MAX);
    free(filename);
    return descriptor;
}

static bool check_symbol(const char* sofile, const char* symbol)
{
    void * dlhandle;
    bool res = false;
    const char* driver_dir;

    if ((driver_dir = getenv("JACK_DRIVER_DIR")) == 0) {
        // for WIN32 ADDON_DIR is defined in JackConstants.h as relative path
        // for posix systems, it is absolute path of default driver dir
#ifdef WIN32
        char temp_driver_dir1[512];
        char temp_driver_dir2[512];
        GetCurrentDirectory(512, temp_driver_dir1);
        sprintf(temp_driver_dir2, "%s/%s", temp_driver_dir1, ADDON_DIR);
        driver_dir = temp_driver_dir2;
#else
        driver_dir = ADDON_DIR;
#endif
    }

    char* filename = (char *)malloc(strlen (driver_dir) + 1 + strlen(sofile) + 1);
    sprintf (filename, "%s/%s", driver_dir, sofile);

    if ((dlhandle = LoadDriverModule(filename)) == NULL) {
#ifdef WIN32
        jack_error ("could not open component .dll '%s': %ld", filename, GetLastError());
#else
        jack_error ("could not open component .so '%s': %s", filename, dlerror());
#endif
     } else {
        res = (GetDriverProc(dlhandle, symbol)) ? true : false;
        UnloadDriverModule(dlhandle);
    }

    free(filename);
    return res;
}

#ifdef WIN32

JSList *
jack_drivers_load (JSList * drivers) {
    char * driver_dir;
    char driver_dir_storage[512];
    char dll_filename[512];
    WIN32_FIND_DATA filedata;
    HANDLE file;
    const char * ptr = NULL;
    JSList * driver_list = NULL;
    jack_driver_desc_t * desc;

    if ((driver_dir = getenv("JACK_DRIVER_DIR")) == 0) {
        // for WIN32 ADDON_DIR is defined in JackConstants.h as relative path
        GetCurrentDirectory(512, driver_dir_storage);
        strcat(driver_dir_storage, "/");
        strcat(driver_dir_storage, ADDON_DIR);
        driver_dir = driver_dir_storage;
    }

    sprintf(dll_filename, "%s/*.dll", driver_dir);

    file = (HANDLE )FindFirstFile(dll_filename, &filedata);

    if (file == INVALID_HANDLE_VALUE) {
        jack_error("error invalid handle");
        return NULL;
    }

    do {
        ptr = strrchr (filedata.cFileName, '.');
        if (!ptr) {
            continue;
        }
        ptr++;
        if (strncmp ("dll", ptr, 3) != 0) {
            continue;
        }

        desc = jack_get_descriptor (drivers, filedata.cFileName, "driver_get_descriptor");
        if (desc) {
            driver_list = jack_slist_append (driver_list, desc);
        } else {
            jack_error ("jack_get_descriptor returns null for \'%s\'", filedata.cFileName);
        }

    } while (FindNextFile(file, &filedata));

    if (!driver_list) {
        jack_error ("could not find any drivers in %s!", driver_dir);
        return NULL;
    }

    return driver_list;
}

#else

JSList *
jack_drivers_load (JSList * drivers) {
    struct dirent * dir_entry;
    DIR * dir_stream;
    const char * ptr;
    int err;
    JSList * driver_list = NULL;
    jack_driver_desc_t * desc;

    const char* driver_dir;
    if ((driver_dir = getenv("JACK_DRIVER_DIR")) == 0) {
        driver_dir = ADDON_DIR;
    }

    /* search through the driver_dir and add get descriptors
    from the .so files in it */
    dir_stream = opendir (driver_dir);
    if (!dir_stream) {
        jack_error ("could not open driver directory %s: %s",
                    driver_dir, strerror (errno));
        return NULL;
    }

    while ((dir_entry = readdir(dir_stream))) {

        /* check the filename is of the right format */
        if (strncmp ("jack_", dir_entry->d_name, 5) != 0) {
            continue;
        }

        ptr = strrchr (dir_entry->d_name, '.');
        if (!ptr) {
            continue;
        }
        ptr++;
        if (strncmp ("so", ptr, 2) != 0) {
            continue;
        }

        desc = jack_get_descriptor (drivers, dir_entry->d_name, "driver_get_descriptor");

        if (desc) {
            driver_list = jack_slist_append (driver_list, desc);
        } else {
            jack_error ("jack_get_descriptor returns null for \'%s\'", dir_entry->d_name);
        }
    }

    err = closedir (dir_stream);
    if (err) {
        jack_error ("error closing driver directory %s: %s",
                    driver_dir, strerror (errno));
    }

    if (!driver_list) {
        jack_error ("could not find any drivers in %s!", driver_dir);
        return NULL;
    }

    return driver_list;
}

#endif

#ifdef WIN32

JSList *
jack_internals_load (JSList * internals) {
    char * driver_dir;
    char driver_dir_storage[512];
    char dll_filename[512];
    WIN32_FIND_DATA filedata;
    HANDLE file;
    const char * ptr = NULL;
    JSList * driver_list = NULL;
    jack_driver_desc_t * desc;

    if ((driver_dir = getenv("JACK_DRIVER_DIR")) == 0) {
        // for WIN32 ADDON_DIR is defined in JackConstants.h as relative path
        GetCurrentDirectory(512, driver_dir_storage);
        strcat(driver_dir_storage, "/");
        strcat(driver_dir_storage, ADDON_DIR);
        driver_dir = driver_dir_storage;
    }

    sprintf(dll_filename, "%s/*.dll", driver_dir);

    file = (HANDLE )FindFirstFile(dll_filename, &filedata);

    if (file == INVALID_HANDLE_VALUE) {
        jack_error("error");
        return NULL;
    }

    do {

        ptr = strrchr (filedata.cFileName, '.');
        if (!ptr) {
            continue;
        }
        ptr++;
        if (strncmp ("dll", ptr, 3) != 0) {
            continue;
        }

        /* check if dll is an internal client */
        if (!check_symbol(filedata.cFileName, "jack_internal_initialize")) {
             continue;
        }

        desc = jack_get_descriptor (internals, filedata.cFileName, "jack_get_descriptor");
        if (desc) {
            driver_list = jack_slist_append (driver_list, desc);
        } else {
            jack_error ("jack_get_descriptor returns null for \'%s\'", filedata.cFileName);
        }

    } while (FindNextFile(file, &filedata));

    if (!driver_list) {
        jack_error ("could not find any internals in %s!", driver_dir);
        return NULL;
    }

    return driver_list;
}

#else

JSList *
jack_internals_load (JSList * internals) {
    struct dirent * dir_entry;
    DIR * dir_stream;
    const char * ptr;
    int err;
    JSList * driver_list = NULL;
    jack_driver_desc_t * desc;

    const char* driver_dir;
    if ((driver_dir = getenv("JACK_DRIVER_DIR")) == 0) {
        driver_dir = ADDON_DIR;
    }

    /* search through the driver_dir and add get descriptors
    from the .so files in it */
    dir_stream = opendir (driver_dir);
    if (!dir_stream) {
        jack_error ("could not open driver directory %s: %s\n",
                    driver_dir, strerror (errno));
        return NULL;
    }

    while ((dir_entry = readdir(dir_stream))) {

        ptr = strrchr (dir_entry->d_name, '.');
        if (!ptr) {
            continue;
        }
        ptr++;
        if (strncmp ("so", ptr, 2) != 0) {
            continue;
        }

        /* check if dll is an internal client */
        if (!check_symbol(dir_entry->d_name, "jack_internal_initialize")) {
             continue;
        }

        desc = jack_get_descriptor (internals, dir_entry->d_name, "jack_get_descriptor");
        if (desc) {
            driver_list = jack_slist_append (driver_list, desc);
        } else {
            jack_error ("jack_get_descriptor returns null for \'%s\'", dir_entry->d_name);
        }
    }

    err = closedir (dir_stream);
    if (err) {
        jack_error ("error closing internal directory %s: %s\n",
                    driver_dir, strerror (errno));
    }

    if (!driver_list) {
        jack_error ("could not find any internals in %s!", driver_dir);
        return NULL;
    }

    return driver_list;
}

#endif

Jack::JackDriverClientInterface* JackDriverInfo::Open(jack_driver_desc_t* driver_desc,
                                                    Jack::JackLockedEngine* engine,
                                                    Jack::JackSynchro* synchro,
                                                    const JSList* params)
{
#ifdef WIN32
    int errstr;
#else
    const char * errstr;
#endif

    fHandle = LoadDriverModule (driver_desc->file);

    if (fHandle == NULL) {
#ifdef WIN32
        if ((errstr = GetLastError ()) != 0) {
            jack_error ("can't load \"%s\": %ld", driver_desc->file, errstr);
#else
        if ((errstr = dlerror ()) != 0) {
            jack_error ("can't load \"%s\": %s", driver_desc->file, errstr);
#endif

        } else {
            jack_error ("bizarre error loading driver shared object %s", driver_desc->file);
        }
        return NULL;
    }

    fInitialize = (driverInitialize)GetDriverProc(fHandle, "driver_initialize");

#ifdef WIN32
    if ((fInitialize == NULL) && (errstr = GetLastError ()) != 0) {
#else
    if ((fInitialize == NULL) && (errstr = dlerror ()) != 0) {
#endif
        jack_error("no initialize function in shared object %s\n", driver_desc->file);
        return NULL;
    }

    fBackend = fInitialize(engine, synchro, params);
    return fBackend;
}

JackDriverInfo::~JackDriverInfo()
{
    delete fBackend;
    if (fHandle)
        UnloadDriverModule(fHandle);
}
