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
#include "JackDriverInfo.h"
#include "JackConstants.h"
#include "JackError.h"
#include <getopt.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#ifndef WIN32
#include <dirent.h>
#endif

#ifdef WIN32

static char* locate_dll_driver_dir()
{
    HMODULE libjack_handle = NULL;
    GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                      reinterpret_cast<LPCSTR>(locate_dll_driver_dir), &libjack_handle);

    // For WIN32 ADDON_DIR is defined in JackConstants.h as relative path
    char driver_dir_storage[512];
    if (3 < GetModuleFileName(libjack_handle, driver_dir_storage, 512)) {
        char *p = strrchr(driver_dir_storage, '\\');
        if (p && (p != driver_dir_storage)) {
            *p = 0;
        }
        jack_info("Drivers/internals found in : %s", driver_dir_storage);
        strcat(driver_dir_storage, "/");
        strcat(driver_dir_storage, ADDON_DIR);
        return strdup(driver_dir_storage);
    } else {
        jack_error("Cannot get JACK dll directory : %d", GetLastError());
        return NULL;
    }
}

static char* locate_driver_dir(HANDLE& file, WIN32_FIND_DATA& filedata)
{
    // Search drivers/internals iin the same folder of "libjackserver.dll"
    char* driver_dir = locate_dll_driver_dir();
    char dll_filename[512];
    snprintf(dll_filename, sizeof(dll_filename), "%s/*.dll", driver_dir);
    file = (HANDLE)FindFirstFile(dll_filename, &filedata);

    if (file == INVALID_HANDLE_VALUE) {
        jack_error("Drivers not found ");
        free(driver_dir);
        return NULL;
    } else {
        return driver_dir;
    }
}

#endif

jack_driver_desc_t* jackctl_driver_get_desc(jackctl_driver_t * driver);

void jack_print_driver_options(jack_driver_desc_t* desc, FILE* file)
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
                if (desc->params[i].value.str && strcmp (desc->params[i].value.str, "") != 0) {
                    sprintf (arg_default, "%s", desc->params[i].value.str);
                } else {
                    sprintf (arg_default, "none");
                }
                break;
            case JackDriverParamBool:
                sprintf (arg_default, "%s", desc->params[i].value.i ? "true" : "false");
                break;
        }

        fprintf(file, "\t-%c, --%s \t%s (default: %s)\n",
                 desc->params[i].character,
                 desc->params[i].name,
                 desc->params[i].long_desc,
                 arg_default);
    }
}

static void jack_print_driver_param_usage (jack_driver_desc_t* desc, unsigned long param, FILE *file)
{
    fprintf (file, "Usage information for the '%s' parameter for driver '%s':\n",
             desc->params[param].name, desc->name);
    fprintf (file, "%s\n", desc->params[param].long_desc);
}

void jack_free_driver_params(JSList * driver_params)
{
    JSList*node_ptr = driver_params;
    JSList*next_node_ptr;

    while (node_ptr) {
        next_node_ptr = node_ptr->next;
        free(node_ptr->data);
        free(node_ptr);
        node_ptr = next_node_ptr;
    }
}

int jack_parse_driver_params(jack_driver_desc_t* desc, int argc, char* argv[], JSList** param_ptr)
{
    struct option * long_options;
    char* options, * options_ptr;
    unsigned long i;
    int opt;
    unsigned int param_index;
    JSList* params = NULL;
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

            fprintf (stderr, "Jackd: unknown option '%s' "
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
                    driver_param->value.i = atoi(optarg);
                    break;
                case JackDriverParamUInt:
                    driver_param->value.ui = strtoul(optarg, NULL, 10);
                    break;
                case JackDriverParamChar:
                    driver_param->value.c = optarg[0];
                    break;
                case JackDriverParamString:
                    strncpy (driver_param->value.str, optarg, JACK_DRIVER_PARAM_STRING_MAX);
                    break;
                case JackDriverParamBool:
                    if (strcasecmp("false", optarg) == 0 ||
                        strcasecmp("off", optarg) == 0 ||
                        strcasecmp("no", optarg) == 0 ||
                        strcasecmp("0", optarg) == 0 ||
                        strcasecmp("(null)", optarg) == 0 ) {
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

    if (param_ptr) {
        *param_ptr = params;
    }
    return 0;
}

SERVER_EXPORT int jackctl_driver_params_parse(jackctl_driver *driver_ptr, int argc, char* argv[])
{
    struct option* long_options;
    char* options, * options_ptr;
    unsigned long i;
    int opt;
    JSList* node_ptr;
    jackctl_parameter_t * param = NULL;
    union jackctl_parameter_value value;

    if (argc <= 1) {
        return 0;
    }

    const JSList* driver_params = jackctl_driver_get_parameters(driver_ptr);
    if (driver_params == NULL) {
        return 1;
    }

    jack_driver_desc_t* desc = jackctl_driver_get_desc(driver_ptr);

    /* check for help */
    if (strcmp (argv[1], "-h") == 0 || strcmp (argv[1], "--help") == 0) {
        if (argc > 2) {
            for (i = 0; i < desc->nparams; i++) {
                if (strcmp (desc->params[i].name, argv[2]) == 0) {
                    jack_print_driver_param_usage (desc, i, stdout);
                    return 1;
                }
            }

            fprintf (stderr, "Jackd: unknown option '%s' "
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
        sprintf(options_ptr, "%c::", desc->params[i].character);
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
                    value.i = atoi(optarg);
                    jackctl_parameter_set_value(param, &value);
                    break;
                case JackDriverParamUInt:
                    value.ui = strtoul(optarg, NULL, 10);
                    jackctl_parameter_set_value(param, &value);
                    break;
                case JackDriverParamChar:
                    value.c = optarg[0];
                    jackctl_parameter_set_value(param, &value);
                    break;
                case JackDriverParamString:
                    strncpy(value.str, optarg, JACK_DRIVER_PARAM_STRING_MAX);
                    jackctl_parameter_set_value(param, &value);
                    break;
                case JackDriverParamBool:
                    if (strcasecmp("false", optarg) == 0 ||
                        strcasecmp("off", optarg) == 0 ||
                        strcasecmp("no", optarg) == 0 ||
                        strcasecmp("0", optarg) == 0 ||
                        strcasecmp("(null)", optarg) == 0 ) {
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

jack_driver_desc_t* jack_find_driver_descriptor (JSList * drivers, const char* name)
{
    jack_driver_desc_t* desc = 0;
    JSList* node;

    for (node = drivers; node; node = jack_slist_next (node)) {
        desc = (jack_driver_desc_t*) node->data;

        if (strcmp (desc->name, name) != 0) {
            desc = NULL;
        } else {
            break;
        }
    }

    return desc;
}

static void* check_symbol(const char* sofile, const char* symbol, const char* driver_dir, void** res_dllhandle = NULL)
{
    void* dlhandle;
    void* res = NULL;
    char filename[1024];
    sprintf(filename, "%s/%s", driver_dir, sofile);

    if ((dlhandle = LoadDriverModule(filename)) == NULL) {
#ifdef WIN32
        jack_error ("Could not open component .dll '%s': %ld", filename, GetLastError());
#else
        jack_error ("Could not open component .so '%s': %s", filename, dlerror());
#endif
    } else {
        res = (void*)GetDriverProc(dlhandle, symbol);
        if (res_dllhandle) {
            *res_dllhandle = dlhandle;
        } else {
            UnloadDriverModule(dlhandle);
        }
    }

    return res;
}

static jack_driver_desc_t* jack_get_descriptor (JSList* drivers, const char* sofile, const char* symbol, const char* driver_dir)
{
    jack_driver_desc_t* descriptor = NULL;
    jack_driver_desc_t* other_descriptor;
    JackDriverDescFunction so_get_descriptor = NULL;
    char filename[1024];
    JSList* node;
    void* dlhandle = NULL;

    sprintf(filename, "%s/%s", driver_dir, sofile);
    so_get_descriptor = (JackDriverDescFunction)check_symbol(sofile, symbol, driver_dir, &dlhandle);

    if (so_get_descriptor == NULL) {
        jack_error("jack_get_descriptor : dll %s is not a driver", sofile);
        goto error;
    }

    if ((descriptor = so_get_descriptor ()) == NULL) {
        jack_error("Driver from '%s' returned NULL descriptor", filename);
        goto error;
    }

    /* check it doesn't exist already */
    for (node = drivers; node; node = jack_slist_next (node)) {
        other_descriptor = (jack_driver_desc_t*) node->data;
        if (strcmp(descriptor->name, other_descriptor->name) == 0) {
            jack_error("The drivers in '%s' and '%s' both have the name '%s'; using the first",
                       other_descriptor->file, filename, other_descriptor->name);
            /* FIXME: delete the descriptor */
            goto error;
        }
    }

    strncpy(descriptor->file, filename, JACK_PATH_MAX);

error:
    if (dlhandle) {
        UnloadDriverModule(dlhandle);
    }
    return descriptor;
}

#ifdef WIN32

JSList * jack_drivers_load(JSList * drivers)
{
    //char dll_filename[512];
    WIN32_FIND_DATA filedata;
    HANDLE file;
    const char* ptr = NULL;
    JSList* driver_list = NULL;
    jack_driver_desc_t* desc = NULL;

    char* driver_dir = locate_driver_dir(file, filedata);
    if (!driver_dir) {
        jack_error("Driver folder not found");
        goto error;
    }

    do {
        /* check the filename is of the right format */
        if (strncmp ("jack_", filedata.cFileName, 5) != 0) {
            continue;
        }

        ptr = strrchr (filedata.cFileName, '.');
        if (!ptr) {
            continue;
        }

        ptr++;
        if (strncmp ("dll", ptr, 3) != 0) {
            continue;
        }

        /* check if dll is an internal client */
        if (check_symbol(filedata.cFileName, "jack_internal_initialize", driver_dir) != NULL) {
            continue;
        }

        desc = jack_get_descriptor (drivers, filedata.cFileName, "driver_get_descriptor", driver_dir);
        if (desc) {
            driver_list = jack_slist_append (driver_list, desc);
        } else {
            jack_error ("jack_get_descriptor returns null for \'%s\'", filedata.cFileName);
        }

    } while (FindNextFile(file, &filedata));

    if (!driver_list) {
        jack_error ("Could not find any drivers in %s!", driver_dir);
    }

error:
    if (driver_dir) {
        free(driver_dir);
    }
    FindClose(file);
    return driver_list;
}

#else

JSList* jack_drivers_load (JSList * drivers)
{
    struct dirent * dir_entry;
    DIR * dir_stream;
    const char* ptr;
    int err;
    JSList* driver_list = NULL;
    jack_driver_desc_t* desc = NULL;

    const char* driver_dir;
    if ((driver_dir = getenv("JACK_DRIVER_DIR")) == 0) {
        driver_dir = ADDON_DIR;
    }

    /* search through the driver_dir and add get descriptors
    from the .so files in it */
    dir_stream = opendir (driver_dir);
    if (!dir_stream) {
        jack_error ("Could not open driver directory %s: %s",
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

        /* check if dll is an internal client */
        if (check_symbol(dir_entry->d_name, "jack_internal_initialize", driver_dir) != NULL) {
            continue;
        }

        desc = jack_get_descriptor (drivers, dir_entry->d_name, "driver_get_descriptor", driver_dir);
        if (desc) {
            driver_list = jack_slist_append (driver_list, desc);
        } else {
            jack_error ("jack_get_descriptor returns null for \'%s\'", dir_entry->d_name);
        }
    }

    err = closedir (dir_stream);
    if (err) {
        jack_error ("Error closing driver directory %s: %s",
                    driver_dir, strerror (errno));
    }

    if (!driver_list) {
        jack_error ("Could not find any drivers in %s!", driver_dir);
        return NULL;
    }

    return driver_list;
}

#endif

#ifdef WIN32

JSList* jack_internals_load(JSList * internals)
{
    ///char dll_filename[512];
    WIN32_FIND_DATA filedata;
    HANDLE file;
    const char* ptr = NULL;
    JSList* driver_list = NULL;
    jack_driver_desc_t* desc;

    char* driver_dir = locate_driver_dir(file, filedata);
    if (!driver_dir) {
        jack_error("Driver folder not found");
        goto error;
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
        if (check_symbol(filedata.cFileName, "jack_internal_initialize", driver_dir) == NULL) {
            continue;
        }

        desc = jack_get_descriptor (internals, filedata.cFileName, "jack_get_descriptor", driver_dir);
        if (desc) {
            driver_list = jack_slist_append (driver_list, desc);
        } else {
            jack_error ("jack_get_descriptor returns null for \'%s\'", filedata.cFileName);
        }

    } while (FindNextFile(file, &filedata));

    if (!driver_list) {
        jack_error ("Could not find any internals in %s!", driver_dir);
    }

 error:
    if (driver_dir) {
        free(driver_dir);
    }
    FindClose(file);
    return driver_list;
}

#else

JSList* jack_internals_load(JSList * internals)
{
    struct dirent * dir_entry;
    DIR * dir_stream;
    const char* ptr;
    int err;
    JSList* driver_list = NULL;
    jack_driver_desc_t* desc;

    const char* driver_dir;
    if ((driver_dir = getenv("JACK_DRIVER_DIR")) == 0) {
        driver_dir = ADDON_DIR;
    }

    /* search through the driver_dir and add get descriptors
    from the .so files in it */
    dir_stream = opendir (driver_dir);
    if (!dir_stream) {
        jack_error ("Could not open driver directory %s: %s\n",
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
        if (check_symbol(dir_entry->d_name, "jack_internal_initialize", driver_dir) == NULL) {
            continue;
        }

        desc = jack_get_descriptor (internals, dir_entry->d_name, "jack_get_descriptor", driver_dir);
        if (desc) {
            driver_list = jack_slist_append (driver_list, desc);
        } else {
            jack_error ("jack_get_descriptor returns null for \'%s\'", dir_entry->d_name);
        }
    }

    err = closedir (dir_stream);
    if (err) {
        jack_error ("Error closing internal directory %s: %s\n",
                    driver_dir, strerror (errno));
    }

    if (!driver_list) {
        jack_error ("Could not find any internals in %s!", driver_dir);
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
    const char* errstr;
#endif

    fHandle = LoadDriverModule (driver_desc->file);

    if (fHandle == NULL) {
#ifdef WIN32
        if ((errstr = GetLastError ()) != 0) {
            jack_error ("Can't load \"%s\": %ld", driver_desc->file, errstr);
#else
        if ((errstr = dlerror ()) != 0) {
            jack_error ("Can't load \"%s\": %s", driver_desc->file, errstr);
#endif

        } else {
            jack_error ("Error loading driver shared object %s", driver_desc->file);
        }
        return NULL;
    }

    fInitialize = (driverInitialize)GetDriverProc(fHandle, "driver_initialize");

#ifdef WIN32
    if ((fInitialize == NULL) && (errstr = GetLastError ()) != 0) {
#else
    if ((fInitialize == NULL) && (errstr = dlerror ()) != 0) {
#endif
        jack_error("No initialize function in shared object %s\n", driver_desc->file);
        return NULL;
    }

    fBackend = fInitialize(engine, synchro, params);
    return fBackend;
}

JackDriverInfo::~JackDriverInfo()
{
    delete fBackend;
    if (fHandle) {
        UnloadDriverModule(fHandle);
    }
}

SERVER_EXPORT jack_driver_desc_t* jack_driver_descriptor_construct(
    const char * name,
    jack_driver_type_t type,
    const char * description,
    jack_driver_desc_filler_t * filler_ptr)
{
    size_t name_len;
    size_t description_len;
    jack_driver_desc_t* desc_ptr;

    name_len = strlen(name);
    description_len = strlen(description);

    if (name_len > sizeof(desc_ptr->name) - 1 ||
        description_len > sizeof(desc_ptr->desc) - 1) {
        assert(false);
        return 0;
    }

    desc_ptr = (jack_driver_desc_t*)calloc (1, sizeof (jack_driver_desc_t));
    if (desc_ptr == NULL) {
        jack_error("Error calloc() failed to allocate memory for driver descriptor struct");
        return 0;
    }

    memcpy(desc_ptr->name, name, name_len + 1);
    memcpy(desc_ptr->desc, description, description_len + 1);

    desc_ptr->nparams = 0;
    desc_ptr->type = type;

    if (filler_ptr != NULL) {
        filler_ptr->size = 0;
    }

    return desc_ptr;
}

SERVER_EXPORT int jack_driver_descriptor_add_parameter(
    jack_driver_desc_t* desc_ptr,
    jack_driver_desc_filler_t * filler_ptr,
    const char* name,
    char character,
    jack_driver_param_type_t type,
    const jack_driver_param_value_t * value_ptr,
    jack_driver_param_constraint_desc_t * constraint,
    const char* short_desc,
    const char* long_desc)
{
    size_t name_len;
    size_t short_desc_len;
    size_t long_desc_len;
    jack_driver_param_desc_t * param_ptr;
    size_t newsize;

    name_len = strlen(name);
    short_desc_len = strlen(short_desc);

    if (long_desc != NULL) {
        long_desc_len = strlen(long_desc);
    } else {
        long_desc = short_desc;
        long_desc_len = short_desc_len;
    }

    if (name_len > sizeof(param_ptr->name) - 1 ||
        short_desc_len > sizeof(param_ptr->short_desc) - 1 ||
        long_desc_len > sizeof(param_ptr->long_desc) - 1) {
        assert(false);
        return 0;
    }

    if (desc_ptr->nparams == filler_ptr->size) {
        newsize = filler_ptr->size + 20; // most drivers have less than 20 parameters
        param_ptr = (jack_driver_param_desc_t*)realloc (desc_ptr->params, newsize * sizeof (jack_driver_param_desc_t));
        if (param_ptr == NULL) {
            jack_error("Error realloc() failed for parameter array of %zu elements", newsize);
            return false;
        }
        filler_ptr->size = newsize;
        desc_ptr->params = param_ptr;
    }

    assert(desc_ptr->nparams < filler_ptr->size);
    param_ptr = desc_ptr->params + desc_ptr->nparams;

    memcpy(param_ptr->name, name, name_len + 1);
    param_ptr->character = character;
    param_ptr->type = type;
    param_ptr->value = *value_ptr;
    param_ptr->constraint = constraint;
    memcpy(param_ptr->short_desc, short_desc, short_desc_len + 1);
    memcpy(param_ptr->long_desc, long_desc, long_desc_len + 1);

    desc_ptr->nparams++;
    return true;
}

SERVER_EXPORT
int
jack_constraint_add_enum(
    jack_driver_param_constraint_desc_t ** constraint_ptr_ptr,
    uint32_t * array_size_ptr,
    jack_driver_param_value_t * value_ptr,
    const char * short_desc)
{
    jack_driver_param_constraint_desc_t * constraint_ptr;
    uint32_t array_size;
    jack_driver_param_value_enum_t * possible_value_ptr;
    size_t len;

    len = strlen(short_desc) + 1;
    if (len > sizeof(possible_value_ptr->short_desc))
    {
        assert(false);
        return false;
    }

    constraint_ptr = *constraint_ptr_ptr;
    if (constraint_ptr == NULL)
    {
        constraint_ptr = (jack_driver_param_constraint_desc_t *)calloc(1, sizeof(jack_driver_param_constraint_desc_t));
        if (constraint_ptr == NULL)
        {
            jack_error("calloc() failed to allocate memory for param constraint struct");
            return false;
        }

        array_size = 0;
    }
    else
    {
        array_size = *array_size_ptr;
    }

    if (constraint_ptr->constraint.enumeration.count == array_size)
    {
        array_size += 10;
        possible_value_ptr =
            (jack_driver_param_value_enum_t *)realloc(
                constraint_ptr->constraint.enumeration.possible_values_array,
                sizeof(jack_driver_param_value_enum_t) * array_size);
        if (possible_value_ptr == NULL)
        {
            jack_error("realloc() failed to (re)allocate memory for possible values array");
            return false;
        }
        constraint_ptr->constraint.enumeration.possible_values_array = possible_value_ptr;
    }
    else
    {
        possible_value_ptr = constraint_ptr->constraint.enumeration.possible_values_array;
    }

    possible_value_ptr += constraint_ptr->constraint.enumeration.count;
    constraint_ptr->constraint.enumeration.count++;

    possible_value_ptr->value = *value_ptr;
    memcpy(possible_value_ptr->short_desc, short_desc, len);

    *constraint_ptr_ptr = constraint_ptr;
    *array_size_ptr = array_size;

    return true;
}

SERVER_EXPORT
void
jack_constraint_free(
    jack_driver_param_constraint_desc_t * constraint_ptr)
{
    if (constraint_ptr != NULL)
    {
        if ((constraint_ptr->flags & JACK_CONSTRAINT_FLAG_RANGE) == 0)
        {
            free(constraint_ptr->constraint.enumeration.possible_values_array);
        }

        free(constraint_ptr);
    }
}

#define JACK_CONSTRAINT_COMPOSE_ENUM_IMPL(type, copy)                   \
JACK_CONSTRAINT_COMPOSE_ENUM(type)                                      \
{                                                                       \
    jack_driver_param_constraint_desc_t * constraint_ptr;               \
    uint32_t array_size;                                                \
    jack_driver_param_value_t value;                                    \
    struct jack_constraint_enum_ ## type ## _descriptor * descr_ptr;    \
                                                                        \
    constraint_ptr = NULL;                                              \
    for (descr_ptr = descr_array_ptr;                                   \
         descr_ptr->value;                                              \
         descr_ptr++)                                                   \
    {                                                                   \
        copy;                                                           \
        if (!jack_constraint_add_enum(                                  \
                &constraint_ptr,                                        \
                &array_size,                                            \
                &value,                                                 \
                descr_ptr->short_desc))                                 \
        {                                                               \
            jack_constraint_free(constraint_ptr);                       \
            return NULL;                                                \
        }                                                               \
    }                                                                   \
                                                                        \
    constraint_ptr->flags = flags;                                      \
                                                                        \
    return constraint_ptr;                                              \
}

JACK_CONSTRAINT_COMPOSE_ENUM_IMPL(uint32, value.c = descr_ptr->value);
JACK_CONSTRAINT_COMPOSE_ENUM_IMPL(sint32, value.c = descr_ptr->value);
JACK_CONSTRAINT_COMPOSE_ENUM_IMPL(char,   value.c = descr_ptr->value);
JACK_CONSTRAINT_COMPOSE_ENUM_IMPL(str, strcpy(value.str, descr_ptr->value));
