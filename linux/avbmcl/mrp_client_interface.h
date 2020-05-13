/*
Copyright (C) 2016-2019 Christoph Kuhr

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

#ifndef _MRP_CLIENT_INTERFACE_H_
#define _MRP_CLIENT_INTERFACE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "avb_definitions.h"

int mrp_client_getDomain_joinVLAN(FILE* filepointer, avb_driver_state_t **avb_ctx, mrp_ctx_t *mrp_ctx);
int mrp_client_listener_await_talker(FILE* filepointer,avb_driver_state_t **avb_ctx, mrp_ctx_t *mrp_ctx);
int mrp_client_listener_send_ready(FILE* filepointer,avb_driver_state_t **avb_ctx, mrp_ctx_t *mrp_ctx);
int mrp_client_listener_send_leave(FILE* filepointer,avb_driver_state_t **avb_ctx, mrp_ctx_t *mrp_ctx);

#ifdef __cplusplus
}
#endif

#endif //_MRP_CLIENT_INTERFACE_H_
