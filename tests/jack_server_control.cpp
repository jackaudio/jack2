/*
 	Copyright (C) 2008 Grame
    
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

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <jack/jack.h>
#include <jack/control.h>

jackctl_server_t * server;

int main(int argc, char *argv[])
{
    const JSList * drivers;
    const JSList * internals;
    const JSList * node_ptr;
     
	server = jackctl_server_create();
    
    drivers = jackctl_server_get_drivers_list(server);
    node_ptr = drivers;
    while (node_ptr != NULL) {
        printf("driver = %s\n", jackctl_driver_get_name((jackctl_driver_t *)node_ptr->data));
        node_ptr = jack_slist_next(node_ptr);
    }
    
    internals = jackctl_server_get_internals_list(server);
    node_ptr = internals;
    while (node_ptr != NULL) {
        printf("internal client = %s\n", jackctl_internal_get_name((jackctl_internal_t *)node_ptr->data));
        node_ptr = jack_slist_next(node_ptr);
    }
    
    jackctl_server_destroy(server);
    return 0;
}
