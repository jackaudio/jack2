/* -*- Mode: C ; c-basic-offset: 4 -*- */
/*
  Copyright (C) 2007 Nedko Arnaudov
    
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#ifndef CONTROLLER_H__2CC80B1E_8D5D_45E3_A9D8_9086DDF68BB5__INCLUDED
#define CONTROLLER_H__2CC80B1E_8D5D_45E3_A9D8_9086DDF68BB5__INCLUDED

void *
jack_controller_create(
    DBusConnection *connection);

void
jack_controller_run(
    void *controller_ptr);

void
jack_controller_destroy(
    void *controller_ptr);

#endif /* #ifndef CONTROLLER_H__2CC80B1E_8D5D_45E3_A9D8_9086DDF68BB5__INCLUDED */
