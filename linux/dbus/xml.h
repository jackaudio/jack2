/* -*- Mode: C ; c-basic-offset: 4 -*- */
/*
    Copyright (C) 2007,2008 Nedko Arnaudov
    
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

#ifndef XML_H__4F102BD2_3354_41C9_B842_DC00E1557A0F__INCLUDED
#define XML_H__4F102BD2_3354_41C9_B842_DC00E1557A0F__INCLUDED

bool
jack_controller_settings_save(
    struct jack_controller * controller_ptr,
    void *dbus_call_context_ptr);

void
jack_controller_settings_load(
    struct jack_controller * controller_ptr);

void
jack_controller_settings_save_auto(
    struct jack_controller * controller_ptr);

#endif /* #ifndef XML_H__4F102BD2_3354_41C9_B842_DC00E1557A0F__INCLUDED */
