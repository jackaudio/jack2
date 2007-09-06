/*
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

#define PRINTDEBUG

#define VERSION "0.66"

#define FORK_SERVER 1

#define BUFFER_SIZE_MAX 8192

#define JACK_PORT_NAME_SIZE 256
#define JACK_PORT_TYPE_SIZE 32

#define JACK_CLIENT_NAME_SIZE 64

#define FIRST_AVAILABLE_PORT 1
#define PORT_NUM 512
#define PORT_NUM_FOR_CLIENT 256

#define CONNECTION_NUM 256

#define CLIENT_NUM 64

#define AUDIO_DRIVER_REFNUM   0						// Audio driver is initialized first, it will get the refnum 0
#define FREEWHEEL_DRIVER_REFNUM   1					// Freewheel driver is initialized second, it will get the refnum 1
#define LOOPBACK_DRIVER_REFNUM   2					// Loopback driver is initialized third, it will get the refnum 2
#define REAL_REFNUM LOOPBACK_DRIVER_REFNUM + 1		// Real clients start at LOOPBACK_DRIVER_REFNUM + 1

#define SOCKET_TIME_OUT 5

#ifdef WIN32
	#define jack_server_dir "server"
	#define jack_client_dir "client"
#elif __APPLE__
	#define jack_server_dir "/tmp"
	#define jack_client_dir "/tmp"
	#define JACK_LOCATION  "/usr/local/bin"
	#define JACK_DEFAULT_DRIVER "coreaudio"
#else
	#define jack_server_dir "/dev/shm"
	#define jack_client_dir "/dev/shm"
	#define JACK_LOCATION  "/usr/local/bin"
	#define JACK_DEFAULT_DRIVER "alsa"
#endif

#define jack_server_entry "jackdmp_entry"
#define jack_client_entry "jack_client"

#define ALL_CLIENTS -1 // for notification

#define JACK_PROTOCOL_VERSION 1
