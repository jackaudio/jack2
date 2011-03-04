/*
    Copyright (C) 2001 Paul Davis
    Copyright (C) 2005 Karsten Wiese, Rui Nuno Capela

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

#include "hardware.h"
#include "alsa_driver.h"
#include "usx2y.h"
#include <sys/mman.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

//#define DBGHWDEP

#ifdef DBGHWDEP
int dbg_offset;
char dbg_buffer[8096];
#endif
static
int usx2y_set_input_monitor_mask (jack_hardware_t *hw, unsigned long mask)
{
	return -1;
}

static
int usx2y_change_sample_clock (jack_hardware_t *hw, SampleClockMode mode)
{
	return -1;
}

static void
usx2y_release (jack_hardware_t *hw)
{
	usx2y_t *h = (usx2y_t *) hw->private_hw;

	if (h == 0)
		return;

	if (h->hwdep_handle)
		snd_hwdep_close(h->hwdep_handle);

	free(h);
}

static int
usx2y_driver_get_channel_addresses_playback (alsa_driver_t *driver,
					snd_pcm_uframes_t *playback_avail)
{
	channel_t chn;
	int iso;
	snd_pcm_uframes_t playback_iso_avail;
	char *playback;

	usx2y_t *h = (usx2y_t *) driver->hw->private_hw;

	if (0 > h->playback_iso_start) {
		int bytes = driver->playback_sample_bytes * 2 * driver->frames_per_cycle *
			driver->user_nperiods;
		iso = h->hwdep_pcm_shm->playback_iso_start;
		if (0 > iso)
			return 0; /* FIXME: return -1; */
		if (++iso >= ARRAY_SIZE(h->hwdep_pcm_shm->captured_iso))
			iso = 0;
		while((bytes -= h->hwdep_pcm_shm->captured_iso[iso].length) > 0)
			if (++iso >= ARRAY_SIZE(h->hwdep_pcm_shm->captured_iso))
				iso = 0;
		h->playback_iso_bytes_done = h->hwdep_pcm_shm->captured_iso[iso].length + bytes;
#ifdef DBGHWDEP
		dbg_offset = sprintf(dbg_buffer, "first iso = %i %i@%p:%i\n",
					iso, h->hwdep_pcm_shm->captured_iso[iso].length,
					h->hwdep_pcm_shm->playback,
					h->hwdep_pcm_shm->captured_iso[iso].offset);
#endif
	} else {
		iso = h->playback_iso_start;
	}
#ifdef DBGHWDEP
	dbg_offset += sprintf(dbg_buffer + dbg_offset, "iso = %i(%i;%i); ", iso,
				h->hwdep_pcm_shm->captured_iso[iso].offset,
				h->hwdep_pcm_shm->captured_iso[iso].frame);
#endif
	playback = h->hwdep_pcm_shm->playback +
		h->hwdep_pcm_shm->captured_iso[iso].offset +
		h->playback_iso_bytes_done;
	playback_iso_avail = (h->hwdep_pcm_shm->captured_iso[iso].length -
		h->playback_iso_bytes_done) /
		(driver->playback_sample_bytes * 2);
	if (*playback_avail >= playback_iso_avail) {
		*playback_avail = playback_iso_avail;
		if (++iso >= ARRAY_SIZE(h->hwdep_pcm_shm->captured_iso))
			iso = 0;
		h->playback_iso_bytes_done = 0;
	} else
		h->playback_iso_bytes_done =
			*playback_avail * (driver->playback_sample_bytes * 2);
	h->playback_iso_start = iso;
	for (chn = 0; chn < driver->playback_nchannels; chn++) {
		const snd_pcm_channel_area_t *a = &driver->playback_areas[chn];
		driver->playback_addr[chn] = playback + a->first / 8;
	}
#ifdef DBGHWDEP
	if (dbg_offset < (sizeof(dbg_buffer) - 256))
		dbg_offset += sprintf(dbg_buffer + dbg_offset, "avail %li@%p\n", *playback_avail, driver->playback_addr[0]);
	else {
		printf(dbg_buffer);
		return -1;
	}
#endif

	return 0;
}

static int
usx2y_driver_get_channel_addresses_capture (alsa_driver_t *driver,
					snd_pcm_uframes_t *capture_avail)
{
	channel_t chn;
	int iso;
	snd_pcm_uframes_t capture_iso_avail;
	int capture_offset;

	usx2y_t *h = (usx2y_t *) driver->hw->private_hw;

	if (0 > h->capture_iso_start) {
		iso = h->hwdep_pcm_shm->capture_iso_start;
		if (0 > iso)
			return 0; /* FIXME: return -1; */
		h->capture_iso_bytes_done = 0;
#ifdef DBGHWDEP
		dbg_offset = sprintf(dbg_buffer, "cfirst iso = %i %i@%p:%i\n",
					iso, h->hwdep_pcm_shm->captured_iso[iso].length,
					h->hwdep_pcm_shm->capture0x8,
					h->hwdep_pcm_shm->captured_iso[iso].offset);
#endif
	} else {
		iso = h->capture_iso_start;
	}
#ifdef DBGHWDEP
	dbg_offset += sprintf(dbg_buffer + dbg_offset, "ciso = %i(%i;%i); ", iso,
				h->hwdep_pcm_shm->captured_iso[iso].offset,
				h->hwdep_pcm_shm->captured_iso[iso].frame);
#endif
	capture_offset =
		h->hwdep_pcm_shm->captured_iso[iso].offset +
			h->capture_iso_bytes_done;
	capture_iso_avail = (h->hwdep_pcm_shm->captured_iso[iso].length -
		h->capture_iso_bytes_done) /
		(driver->capture_sample_bytes * 2);
	if (*capture_avail >= capture_iso_avail) {
		*capture_avail = capture_iso_avail;
		if (++iso >= ARRAY_SIZE(h->hwdep_pcm_shm->captured_iso))
			iso = 0;
		h->capture_iso_bytes_done = 0;
	} else
		h->capture_iso_bytes_done =
			*capture_avail * (driver->capture_sample_bytes * 2);
	h->capture_iso_start = iso;
	for (chn = 0; chn < driver->capture_nchannels; chn++) {
		driver->capture_addr[chn] =
			(chn < 2 ? h->hwdep_pcm_shm->capture0x8 : h->hwdep_pcm_shm->capture0xA)
			+ capture_offset +
			((chn & 1) ? driver->capture_sample_bytes : 0);
	}
#ifdef DBGHWDEP
 {
	int f = 0;
	unsigned *u = driver->capture_addr[0];
	static unsigned last;
	dbg_offset += sprintf(dbg_buffer + dbg_offset, "\nvon %6u  bis %6u\n", last, u[0]);
	while (f < *capture_avail && dbg_offset < (sizeof(dbg_buffer) - 256)) {
		if (u[f] != last + 1)
			 dbg_offset += sprintf(dbg_buffer + dbg_offset, "\nooops %6u  %6u\n", last, u[f]);
		last = u[f++];
	}
 }
	if (dbg_offset < (sizeof(dbg_buffer) - 256))
		dbg_offset += sprintf(dbg_buffer + dbg_offset, "avail %li@%p\n", *capture_avail, driver->capture_addr[0]);
	else {
		printf(dbg_buffer);
		return -1;
	}
#endif

	return 0;
}

static int
usx2y_driver_start (alsa_driver_t *driver)
{
	int err, i;
	snd_pcm_uframes_t poffset, pavail;

	usx2y_t *h = (usx2y_t *) driver->hw->private_hw;

	for (i = 0; i < driver->capture_nchannels; i++)
		// US428 channels 3+4 are on a seperate 2 channel stream.
		// ALSA thinks its 1 stream with 4 channels.
		driver->capture_interleave_skip[i] = 2 * driver->capture_sample_bytes;


	driver->playback_interleave_skip[0] = 2 * driver->playback_sample_bytes;
	driver->playback_interleave_skip[1] = 2 * driver->playback_sample_bytes;

	driver->poll_last = 0;
	driver->poll_next = 0;

	if ((err = snd_pcm_prepare (driver->playback_handle)) < 0) {
		jack_error ("ALSA/USX2Y: prepare error for playback: %s", snd_strerror(err));
		return -1;
	}

	if (driver->midi && !driver->xrun_recovery)
		(driver->midi->start)(driver->midi);

	if (driver->playback_handle) {
/* 		int i, j; */
/* 		char buffer[2000]; */
		h->playback_iso_start =
			h->capture_iso_start = -1;
		snd_hwdep_poll_descriptors(h->hwdep_handle, &h->pfds, 1);
		h->hwdep_pcm_shm = (snd_usX2Y_hwdep_pcm_shm_t*)
			mmap(NULL, sizeof(snd_usX2Y_hwdep_pcm_shm_t),
			     PROT_READ,
			     MAP_SHARED, h->pfds.fd,
			     0);
		if (MAP_FAILED == h->hwdep_pcm_shm) {
			perror("ALSA/USX2Y: mmap");
			return -1;
		}
		if (mprotect(h->hwdep_pcm_shm->playback,
					sizeof(h->hwdep_pcm_shm->playback),
					PROT_READ|PROT_WRITE)) {
			perror("ALSA/USX2Y: mprotect");
			return -1;
		}
		memset(h->hwdep_pcm_shm->playback, 0, sizeof(h->hwdep_pcm_shm->playback));
/* 		for (i = 0, j = 0; i < 2000;) { */
/* 			j += sprintf(buffer + j, "%04hX ", */
/* 				     *(unsigned short*)(h->hwdep_pcm_shm->capture + i)); */
/* 			if (((i += 2) % 32) == 0) { */
/* 				jack_error(buffer); */
/* 				j = 0; */
/* 			} */
/* 		} */
	}

	if (driver->hw_monitoring) {
		driver->hw->set_input_monitor_mask (driver->hw,
						    driver->input_monitor_mask);
	}

	if (driver->playback_handle) {
		/* fill playback buffer with zeroes, and mark
		   all fragments as having data.
		*/

		pavail = snd_pcm_avail_update (driver->playback_handle);

		if (pavail != driver->frames_per_cycle * driver->playback_nperiods) {
			jack_error ("ALSA/USX2Y: full buffer not available at start");
			return -1;
		}

		if (snd_pcm_mmap_begin(
					driver->playback_handle,
					&driver->playback_areas,
					&poffset, &pavail) < 0) {
			return -1;
		}

		/* XXX this is cheating. ALSA offers no guarantee that
		   we can access the entire buffer at any one time. It
		   works on most hardware tested so far, however, buts
		   its a liability in the long run. I think that
		   alsa-lib may have a better function for doing this
		   here, where the goal is to silence the entire
		   buffer.
		*/
		{
/* 			snd_pcm_uframes_t frag, nframes = driver->buffer_frames; */
/* 			while (nframes) { */
/* 				frag = nframes; */
/* 				if (usx2y_driver_get_channel_addresses_playback(driver, &frag) < 0) */
/* 					return -1; */

/* 				for (chn = 0; chn < driver->playback_nchannels; chn++) */
/* 					alsa_driver_silence_on_channel (driver, chn, frag); */
/* 				nframes -= frag; */
/* 			} */
		}

		snd_pcm_mmap_commit (driver->playback_handle, poffset,
						driver->user_nperiods * driver->frames_per_cycle);

		if ((err = snd_pcm_start (driver->playback_handle)) < 0) {
			jack_error ("ALSA/USX2Y: could not start playback (%s)",
				    snd_strerror (err));
			return -1;
		}
	}

	if (driver->hw_monitoring &&
	    (driver->input_monitor_mask || driver->all_monitor_in)) {
		if (driver->all_monitor_in) {
			driver->hw->set_input_monitor_mask (driver->hw, ~0U);
		} else {
			driver->hw->set_input_monitor_mask (
				driver->hw, driver->input_monitor_mask);
		}
	}

	driver->playback_nfds =	snd_pcm_poll_descriptors_count (driver->playback_handle);
	driver->capture_nfds = snd_pcm_poll_descriptors_count (driver->capture_handle);

	if (driver->pfd) {
		free (driver->pfd);
	}

	driver->pfd = (struct pollfd *)
		malloc (sizeof (struct pollfd) *
			(driver->playback_nfds + driver->capture_nfds + 2));

	return 0;
}

static int
usx2y_driver_stop (alsa_driver_t *driver)
{
	int err;
	JSList* node;
	int chn;

	usx2y_t *h = (usx2y_t *) driver->hw->private_hw;

	/* silence all capture port buffers, because we might
	   be entering offline mode.
	*/

	for (chn = 0, node = driver->capture_ports; node;
		node = jack_slist_next (node), chn++) {

		jack_port_t* port;
		char* buf;
		jack_nframes_t nframes = driver->engine->control->buffer_size;

		port = (jack_port_t *) node->data;
		buf = jack_port_get_buffer (port, nframes);
		memset (buf, 0, sizeof (jack_default_audio_sample_t) * nframes);
	}

	if (driver->playback_handle) {
		if ((err = snd_pcm_drop (driver->playback_handle)) < 0) {
			jack_error ("ALSA/USX2Y: channel flush for playback "
					"failed (%s)", snd_strerror (err));
			return -1;
		}
	}

	if (driver->hw_monitoring) {
		driver->hw->set_input_monitor_mask (driver->hw, 0);
	}

	munmap(h->hwdep_pcm_shm, sizeof(snd_usX2Y_hwdep_pcm_shm_t));

	if (driver->midi && !driver->xrun_recovery)
		(driver->midi->stop)(driver->midi);

	return 0;
}

static int
usx2y_driver_null_cycle (alsa_driver_t* driver, jack_nframes_t nframes)
{
	jack_nframes_t nf;
	snd_pcm_uframes_t offset;
	snd_pcm_uframes_t contiguous, contiguous_;
	int chn;

	VERBOSE(driver->engine,
		"usx2y_driver_null_cycle (%p, %i)", driver, nframes);

	if (driver->capture_handle) {
		nf = nframes;
		offset = 0;
		while (nf) {

			contiguous = (nf > driver->frames_per_cycle) ?
				driver->frames_per_cycle : nf;

			if (snd_pcm_mmap_begin (
					driver->capture_handle,
					&driver->capture_areas,
					(snd_pcm_uframes_t *) &offset,
					(snd_pcm_uframes_t *) &contiguous)) {
				return -1;
			}
			contiguous_ = contiguous;
			while (contiguous_) {
				snd_pcm_uframes_t frag = contiguous_;
				if (usx2y_driver_get_channel_addresses_capture(driver, &frag) < 0)
					return -1;
				contiguous_ -= frag;
			}

			if (snd_pcm_mmap_commit (driver->capture_handle,
						offset, contiguous) < 0) {
				return -1;
			}

			nf -= contiguous;
		}
	}

	if (driver->playback_handle) {
		nf = nframes;
		offset = 0;
		while (nf) {
			contiguous = (nf > driver->frames_per_cycle) ?
				driver->frames_per_cycle : nf;

			if (snd_pcm_mmap_begin (
				    driver->playback_handle,
				    &driver->playback_areas,
				    (snd_pcm_uframes_t *) &offset,
				    (snd_pcm_uframes_t *) &contiguous)) {
				return -1;
			}

			{
				snd_pcm_uframes_t frag, nframes = contiguous;
				while (nframes) {
					frag = nframes;
					if (usx2y_driver_get_channel_addresses_playback(driver, &frag) < 0)
						return -1;
					for (chn = 0; chn < driver->playback_nchannels; chn++)
						alsa_driver_silence_on_channel (driver, chn, frag);
					nframes -= frag;
				}
			}

			if (snd_pcm_mmap_commit (driver->playback_handle,
						offset, contiguous) < 0) {
				return -1;
			}

			nf -= contiguous;
		}
	}

	return 0;
}

static int
usx2y_driver_read (alsa_driver_t *driver, jack_nframes_t nframes)
{
	snd_pcm_uframes_t contiguous;
	snd_pcm_sframes_t nread;
	snd_pcm_uframes_t offset;
	jack_default_audio_sample_t* buf[4];
	channel_t chn;
	JSList *node;
	jack_port_t* port;
	int err;
	snd_pcm_uframes_t nframes_ = nframes;

	if (!driver->capture_handle || driver->engine->freewheeling) {
		return 0;
	}

    if (driver->midi)
        (driver->midi->read)(driver->midi, nframes);

	nread = 0;

	if (snd_pcm_mmap_begin (driver->capture_handle,
				&driver->capture_areas,
				&offset, &nframes_) < 0) {
		jack_error ("ALSA/USX2Y: %s: mmap areas info error",
			    driver->alsa_name_capture);
		return -1;
	}

	for (chn = 0, node = driver->capture_ports;
	     node; node = jack_slist_next (node), chn++) {
		port = (jack_port_t *) node->data;
		if (!jack_port_connected (port)) {
			continue;
		}
		buf[chn] = jack_port_get_buffer (port, nframes_);
	}

	while (nframes) {

		contiguous = nframes;
		if (usx2y_driver_get_channel_addresses_capture (
			    driver, &contiguous) < 0) {
			return -1;
		}
		for (chn = 0, node = driver->capture_ports;
		     node; node = jack_slist_next (node), chn++) {
			port = (jack_port_t *) node->data;
			if (!jack_port_connected (port)) {
				/* no-copy optimization */
				continue;
			}
			alsa_driver_read_from_channel (driver, chn,
						       buf[chn] + nread,
						       contiguous);
/* 			sample_move_dS_s24(buf[chn] + nread, */
/* 					   driver->capture_addr[chn], */
/* 					   contiguous, */
/* 					   driver->capture_interleave_skip); */
		}
		nread += contiguous;
		nframes -= contiguous;
	}

	if ((err = snd_pcm_mmap_commit (driver->capture_handle,
					offset, nframes_)) < 0) {
		jack_error ("ALSA/USX2Y: could not complete read of %"
			    PRIu32 " frames: error = %d", nframes_, err);
		return -1;
	}

	return 0;
}

static int
usx2y_driver_write (alsa_driver_t* driver, jack_nframes_t nframes)
{
	channel_t chn;
	JSList *node;
	jack_default_audio_sample_t* buf[2];
	snd_pcm_sframes_t nwritten;
	snd_pcm_uframes_t contiguous;
	snd_pcm_uframes_t offset;
	jack_port_t *port;
	int err;
	snd_pcm_uframes_t nframes_ = nframes;

	driver->process_count++;

	if (!driver->playback_handle || driver->engine->freewheeling) {
		return 0;
	}

    if (driver->midi)
        (driver->midi->write)(driver->midi, nframes);

	nwritten = 0;

	/* check current input monitor request status */

	driver->input_monitor_mask = 0;

	for (chn = 0, node = driver->capture_ports; node;
		node = jack_slist_next (node), chn++) {
		if (((jack_port_t *) node->data)->shared->monitor_requests) {
			driver->input_monitor_mask |= (1<<chn);
		}
	}

	if (driver->hw_monitoring) {
		if ((driver->hw->input_monitor_mask
			!= driver->input_monitor_mask)
			&& !driver->all_monitor_in) {
			driver->hw->set_input_monitor_mask (
				driver->hw, driver->input_monitor_mask);
		}
	}

	if (snd_pcm_mmap_begin(driver->playback_handle,
			       &driver->playback_areas,
			       &offset, &nframes_) < 0) {
		jack_error ("ALSA/USX2Y: %s: mmap areas info error",
			    driver->alsa_name_capture);
		return -1;
	}

	for (chn = 0, node = driver->playback_ports;
	     node; node = jack_slist_next (node), chn++) {
		port = (jack_port_t *) node->data;
		buf[chn] = jack_port_get_buffer (port, nframes_);
	}

	while (nframes) {

		contiguous = nframes;
		if (usx2y_driver_get_channel_addresses_playback (
			    driver, &contiguous) < 0) {
			return -1;
		}
		for (chn = 0, node = driver->playback_ports;
		     node; node = jack_slist_next (node), chn++) {
			port = (jack_port_t *) node->data;
			alsa_driver_write_to_channel (driver, chn,
						      buf[chn] + nwritten,
						      contiguous);
		}
		nwritten += contiguous;
		nframes -= contiguous;
	}

	if ((err = snd_pcm_mmap_commit (driver->playback_handle,
					offset, nframes_)) < 0) {
		jack_error ("ALSA/USX2Y: could not complete playback of %"
			    PRIu32 " frames: error = %d", nframes_, err);
		if (err != -EPIPE && err != -ESTRPIPE)
			return -1;
	}

	return 0;
}

static void
usx2y_driver_setup (alsa_driver_t *driver)
{
	driver->nt_start = (JackDriverNTStartFunction) usx2y_driver_start;
	driver->nt_stop = (JackDriverNTStopFunction) usx2y_driver_stop;
	driver->read = (JackDriverReadFunction) usx2y_driver_read;
	driver->write = (JackDriverReadFunction) usx2y_driver_write;
	driver->null_cycle =
		(JackDriverNullCycleFunction) usx2y_driver_null_cycle;
}

jack_hardware_t *
jack_alsa_usx2y_hw_new (alsa_driver_t *driver)
{
	jack_hardware_t *hw;
	usx2y_t *h;

	int   hwdep_cardno;
    int   hwdep_devno;
	char *hwdep_colon;
	char  hwdep_name[9];
	snd_hwdep_t *hwdep_handle;

    hw = (jack_hardware_t *) malloc (sizeof (jack_hardware_t));

	hw->capabilities = 0;
	hw->input_monitor_mask = 0;
	hw->private_hw = 0;

	hw->set_input_monitor_mask = usx2y_set_input_monitor_mask;
	hw->change_sample_clock = usx2y_change_sample_clock;
	hw->release = usx2y_release;

	/* Derive the special USB US-X2Y hwdep pcm device name from
	 * the playback one, thus allowing the use of the "rawusb"
	 * experimental stuff if, and only if, the "hw:n,2" device
	 * name is specified. Otherwise, fallback to generic backend.
	 */
	hwdep_handle = NULL;
	hwdep_cardno = hwdep_devno = 0;
	if ((hwdep_colon = strrchr(driver->alsa_name_playback, ':')) != NULL)
		sscanf(hwdep_colon, ":%d,%d", &hwdep_cardno, &hwdep_devno);
	if (hwdep_devno == 2) {
		snprintf(hwdep_name, sizeof(hwdep_name), "hw:%d,1", hwdep_cardno);
		if (snd_hwdep_open (&hwdep_handle, hwdep_name, O_RDWR) < 0) {
			jack_error ("ALSA/USX2Y: Cannot open hwdep device \"%s\"", hwdep_name);
		} else {
			/* Allocate specific USX2Y hwdep pcm struct. */
			h = (usx2y_t *) malloc (sizeof (usx2y_t));
			h->driver = driver;
			h->hwdep_handle = hwdep_handle;
			hw->private_hw = h;
			/* Set our own operational function pointers. */
			usx2y_driver_setup(driver);
			jack_info("ALSA/USX2Y: EXPERIMENTAL hwdep pcm device %s"
				" (aka \"rawusb\")", driver->alsa_name_playback);
		}
	}

	return hw;
}
