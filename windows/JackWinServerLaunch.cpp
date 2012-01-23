/*
Copyright (C) 2001-2003 Paul Davis
Copyright (C) 2004-2008 Grame
Copyright (C) 2011 John Emmas

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

#include "JackChannel.h"
#include "JackLibGlobals.h"
#include "JackServerLaunch.h"
#include "JackPlatformPlug.h"

using namespace Jack;

#include <shlobj.h>
#include <process.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>

#if defined(_MSC_VER) || defined(__MINGW__) || defined(__MINGW32__)

static char*
find_path_to_jackdrc(char *path_to_jackdrc)
{
    char user_jackdrc[1024];
    char *ret = NULL;

	user_jackdrc[0] = user_jackdrc[1] = 0; // Initialise

	if (S_OK == SHGetFolderPath(NULL, CSIDL_PROFILE, NULL, SHGFP_TYPE_CURRENT, user_jackdrc))
	{
		// The above call should have given us the path to the user's home folder
		char ch = user_jackdrc[strlen(user_jackdrc)-1];

		if (('/' != ch) && ('\\' != ch))
			strcat(user_jackdrc, "\\");

		if (user_jackdrc[1] == ':')
		{
			// Assume we have a valid path
			strcat(user_jackdrc, ".jackdrc");
			strcpy(path_to_jackdrc, user_jackdrc);

			ret = path_to_jackdrc;
		}
		else
			path_to_jackdrc[0] = '\0';
	}
	else
		path_to_jackdrc[0] = '\0';

	return (ret);
}

#else

static char*
find_path_to_jackdrc(char *path_to_jackdrc)
{
	return 0;
}

#endif

/* 'start_server_aux()' - this function might need to be modified (though probably
 * not) to cope with compilers other than MSVC (e.g. MinGW). The function
 * 'find_path_to_jackdrc()' might also need to be written for MinGW, though for
 * Cygwin, JackPosixServerLaunch.cpp can be used instead of this file.
 */

#include <direct.h>

static int start_server_aux(const char* server_name)
{
    FILE*  fp      = 0;
    size_t pos     = 0;
    size_t result  = 0;
    int    i       = 0;
    int    good    = 0;
    int    ret     = 0;
    char*  command = 0;
    char** argv    = 0;
    char*  p;
    char*  back_slash;
    char*  forward_slash;
    char   arguments [256];
    char   buffer    [MAX_PATH];
    char   filename  [MAX_PATH];
    char   curr_wd   [MAX_PATH];

	curr_wd[0] = '\0';
	if (find_path_to_jackdrc(filename))
		fp = fopen(filename, "r");

	/* if still not found, check old config name for backwards compatability */
	/* JE - hopefully won't be needed for the Windows build
    if (!fp) {
        fp = fopen("/etc/jackd.conf", "r");
    }
    */

	if (fp) {
		arguments[0] = '\0';

		fgets(filename, MAX_PATH, fp);
		_strlwr(filename);
		if ((p = strstr(filename, ".exe"))) {
			p += 4;
			*p = '\0';
			pos = (size_t)(p - filename);
			fseek(fp, 0, SEEK_SET);

			if ((command = (char*)malloc(pos+1)))
				ret = fread(command, 1, pos, fp);

			if (ret && !ferror(fp)) {
				command[pos]  = '\0'; // NULL terminator
				back_slash    = strrchr(command, '\\');
				forward_slash = strrchr(command, '/');
				if (back_slash > forward_slash)
					p = back_slash + 1;
				else
					p = forward_slash + 1;

				strcpy(buffer, p);
				while (ret != 0 && ret != EOF) {
					strcat(arguments, buffer);
					strcat(arguments, " ");
					ret = fscanf(fp, "%s", buffer);
				}

				if (strlen(arguments) > 0) {
					good = 1;
				}
			}
		}

		fclose(fp);
	}

    if (!good) {
		strcpy(buffer, JACK_LOCATION "/jackd.exe");
		command = (char*)malloc((strlen(buffer))+1);
		strcpy(command, buffer);
        strncpy(arguments, "jackd.exe -S -d " JACK_DEFAULT_DRIVER, 255);
    }

    int  buffer_termination;
    bool verbose_mode = false;
    argv = (char**)malloc(255);
    pos  = 0;

    while (1) {
        /* insert -T and -n server_name in front of arguments */
        if (i == 1) {
            argv[i] = (char*)malloc(strlen ("-T") + 1);
            strcpy (argv[i++], "-T");
            if (server_name) {
                size_t optlen = strlen("-n");
                char* buf = (char*)malloc(optlen + strlen(server_name) + 1);
                strcpy(buf, "-n");
                strcpy(buf + optlen, server_name);
                argv[i++] = buf;
            }
        }

		// Only get the next character if there's more than 1 character
		if ((pos < strlen(arguments)) && (arguments[pos+1]) && (arguments[pos+1] != ' ')) {
			strncpy(buffer, arguments + pos++, 1);
			buffer_termination = 1;
		} else {
			buffer[0] = '\0';
			buffer_termination = 0;
		}

		buffer[1] = '\0';
		if (buffer[0] == '\"')
			result = strcspn(arguments + pos, "\"");
		else
			result = strcspn(arguments + pos, " ");

        if (0 == result)
            break;
		else
		{
			strcat(buffer, arguments + pos);

			// Terminate the buffer
			buffer[result + buffer_termination] = '\0';
			if (buffer[0] == '\"') {
				strcat(buffer, "\"");
				++result;
			}

			argv[i] = (char*)malloc(strlen(buffer) + 1);
			strcpy(argv[i], buffer);
			pos += (result + 1);
			++i;

			if ((0 == strcmp(buffer, "-v")) || (0 == strcmp(buffer, "--verbose")))
				verbose_mode = true;
		}
    }

	argv[i] = 0;

#ifdef SUPPORT_PRE_1_9_8_SERVER
	// Get the current working directory
	if (_getcwd(curr_wd, MAX_PATH)) {
		strcpy(temp_wd, command);
		back_slash    = strrchr(temp_wd, '\\');
		forward_slash = strrchr(temp_wd, '/');
		if (back_slash > forward_slash)
			p = back_slash;
		else
			p = forward_slash;
		*p = '\0';

		// Accommodate older versions of Jack (pre v1.9.8) which
		// might need to be started from their installation folder.
		_chdir(temp_wd);
	}
#endif

	if (verbose_mode) {
		// Launch the server with a console... (note that
		// if the client is a console app, the server might
		// also use the client's console)
		ret = _spawnv(_P_NOWAIT, command, argv);
	} else {
		// Launch the server silently... (without a console)
		ret = _spawnv(_P_DETACH, command, argv);
	}

    Sleep(2500); // Give it some time to launch

	if ((-1) == ret)
		fprintf(stderr, "Execution of JACK server (command = \"%s\") failed: %s\n", command, strerror(errno));

	if (strlen(curr_wd)) {
		// Change the cwd back to its original setting
		_chdir(curr_wd);
	}

	if (command)
		free(command);

	if (argv) {
		for (i = 0; argv[i] != 0; i++)
			free (argv[i]);

		free(argv);
	}

	return (ret == (-1) ? false : true);
}

static int start_server(const char* server_name, jack_options_t options)
{
	if ((options & JackNoStartServer) || getenv("JACK_NO_START_SERVER")) {
		return 1;
	}

	return (((-1) != (start_server_aux(server_name)) ? 0 : (-1)));
}

static int server_connect(const char* server_name)
{
	JackClientChannel channel;
	int res = channel.ServerCheck(server_name);
	channel.Close();
	/*
	JackSleep(2000); // Added by JE - 02-01-2009 (gives
	                 // the channel some time to close)
	                 */
    JackSleep(500);
	return res;
}

int try_start_server(jack_varargs_t* va, jack_options_t options, jack_status_t* status)
{
	if (server_connect(va->server_name) < 0) {
		int trys;
		if (start_server(va->server_name, options)) {
			int my_status1 = *status | JackFailure | JackServerFailed;
			*status = (jack_status_t)my_status1;
			return -1;
		}
		trys = 5;
		do {
			Sleep(1000);
			if (--trys < 0) {
				int my_status1 = *status | JackFailure | JackServerFailed;
				*status = (jack_status_t)my_status1;
				return -1;
			}
		} while (server_connect(va->server_name) < 0);
		int my_status1 = *status | JackServerStarted;
		*status = (jack_status_t)my_status1;
	}

	return 0;
}
