/*
Copyright (C) 2007 Grame

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

#ifndef __JackNotification__
#define __JackNotification__

namespace Jack
{

/*!
\brief Notifications sent by the server for clients.
*/

enum NotificationType {
    kAddClient = 0,
    kRemoveClient = 1,
    kActivateClient = 2,
    kXRunCallback = 3,
    kGraphOrderCallback = 4,
    kBufferSizeCallback = 5,
    kSampleRateCallback = 6,
    kStartFreewheelCallback = 7,
    kStopFreewheelCallback = 8,
    kPortRegistrationOnCallback = 9,
    kPortRegistrationOffCallback = 10,
    kPortConnectCallback = 11,
    kPortDisconnectCallback = 12,
    kPortRenameCallback = 13,
    kRealTimeCallback = 14,
    kShutDownCallback = 15,
    kQUIT = 16,
    kSessionCallback = 17,
    kLatencyCallback = 18,
    kMaxNotification
};

} // end of namespace

#endif
