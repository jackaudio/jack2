/*
Copyright (C) 2007 Grame  

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

#ifndef __JackNotification__
#define __JackNotification__

namespace Jack
{
	enum NotificationType {
		kAddClient = 0,
		kRemoveClient = 1,
		kActivateClient = 2,
		kXRunCallback = 3,
		kGraphOrderCallback = 4,
		kBufferSizeCallback = 5,
		kStartFreewheelCallback = 6,
		kStopFreewheelCallback = 7,
		kPortRegistrationOnCallback = 8,
		kPortRegistrationOffCallback = 9,
		kPortConnectCallback = 10,
		kPortDisconnectCallback = 11,
		kDeadClient = 12,
		kMaxNotification
	};

} // end of namespace

#endif
