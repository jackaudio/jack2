/*
    Copyright (C) 2002-2003 Paul Davis

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

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "timestamps.h"
#include "JackTime.h"

typedef struct {
    jack_time_t when;
    const char *what;
} jack_timestamp_t;

static jack_timestamp_t *timestamps = 0;
static unsigned long timestamp_cnt = 0;
static unsigned long timestamp_index;

void
jack_init_timestamps (unsigned long howmany)
{
	if (timestamps) {
		free (timestamps);
	}
	timestamps = (jack_timestamp_t *)
		malloc (howmany * sizeof(jack_timestamp_t));
	timestamp_cnt = howmany;
	memset (timestamps, 0, sizeof(jack_timestamp_t) * howmany);
	timestamp_index = 0;
}

void
jack_timestamp (const char *what)
{
	if (timestamp_index < timestamp_cnt) {
		timestamps[timestamp_index].when = GetMicroSeconds();
		timestamps[timestamp_index].what = what;
		++timestamp_index;
	}
}

void
jack_dump_timestamps (FILE *out)
{
	unsigned long i;

	for (i = 0; i < timestamp_index; ++i) {
		fprintf (out, "%-.32s %" PRIu64 " %" PRIu64,
			 timestamps[i].what, timestamps[i].when,
			 timestamps[i].when - timestamps[0].when);
		if (i > 0) {
			fprintf (out, " %" PRIu64,
				 timestamps[i].when - timestamps[i-1].when);
		}
		fputc ('\n', out);
	}
}

void
jack_reset_timestamps ()
{
	timestamp_index = 0;
}

