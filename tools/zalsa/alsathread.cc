// ----------------------------------------------------------------------------
//
//  Copyright (C) 2012-2018 Fons Adriaensen <fons@linuxaudio.org>
//    
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// ----------------------------------------------------------------------------


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "alsathread.h"
#include "timers.h"


Alsathread::Alsathread (Alsa_pcmi *alsadev, int mode) :
    _alsadev (alsadev ),
    _mode (mode),
    _state (INIT),
    _fsize (alsadev->fsize ()),
    _audioq (0),
    _commq (0),
    _alsaq (0)
{
    // Compute DLL filter coefficients.
    _dt = (double) _fsize / _alsadev->fsamp ();
    _w1 = 2 * M_PI * 0.1 * _dt;
    _w2 = _w1 * _w1;
    _w1 *= 1.6;
}


Alsathread::~Alsathread (void)
{
    if (_state != INIT)
    {
        _state = TERM;
        thr_wait ();
    }
    else
    {
        _alsadev->pcm_stop ();
    }
}


int Alsathread::start (Lfq_audio *audioq, Lfq_int32 *commq, Lfq_adata *alsaq, int rtprio)
{
    // Start the ALSA thread.
    _audioq = audioq;
    _commq = commq;
    _alsaq = alsaq;
    _state = WAIT;
    if (thr_start (SCHED_FIFO, rtprio, 0x10000)) return 1;
    return 0;
}


void Alsathread::send (int k, double t)
{
    Adata *D;

    // Send (state, frame count, timestamp) to Jack thread.
    if (_alsaq->wr_avail ())
    {
        D = _alsaq->wr_datap (); 
        D->_state = _state;
        D->_nsamp = k;
        D->_timer = t;
        _alsaq->wr_commit ();
    }
}


// The following two functions transfer data between the audio queue
// and the ALSA device. Note that we do *not* check the queue's fill
// state, and it may overrun or underrun. It actually will in the first
// few iterations and in error conditions. This is entirely intentional.
// The queue keeps correct read and write counters even in that case,
// and the  main control loop and error recovery depend on it working
// and being used in this way. 

int Alsathread::capture (void)
{
    int    c, n, k;
    float  *p;

    // Start reading from ALSA device.
    _alsadev->capt_init (_fsize);
    if (_state == PROC)
    {
	// Input frames from the ALSA device to the audio queue.
	// The outer loop takes care of wraparound.
	for (n = _fsize; n; n -= k)
	{
	    p = _audioq->wr_datap ();  // Audio queue write pointer.
	    k = _audioq->wr_linav ();  // Number of frames that can be
	    if (k > n) k = n;          // written without wraparound.
	    for (c = 0; c < _audioq->nchan (); c++)
	    {
		// Copy and interleave one channel.
		_alsadev->capt_chan (c, p + c, k, _audioq->nchan ());
	    }
	    _audioq->wr_commit (k);    // Update audio queue state.
	}
    }
    // Finish reading from ALSA device.
    _alsadev->capt_done (_fsize);
    return _fsize;
}


int Alsathread::playback (void)
{
    int    c, n, k;
    float  *p;

    // Start writing to ALSA device.
    _alsadev->play_init (_fsize);
    c = 0;
    if (_state == PROC)
    {
	// Output frames from the audio queue to the ALSA device.
	// The outer loop takes care of wraparound. 
	for (n = _fsize; n; n -= k)
	{
	    p = _audioq->rd_datap ();  // Audio queue read pointer.
	    k = _audioq->rd_linav ();  // Number of frames that can
	    if (k > n) k = n;          // be read without wraparound.
	    for (c = 0; c < _audioq->nchan (); c++)
	    {
		// De-interleave and copy one channel.
		_alsadev->play_chan (c, p + c, k, _audioq->nchan ());
	    }
	    _audioq->rd_commit (k);    // Update audio queue state.
	}
    }
    // Clear all or remaining channels.
    while (c < _alsadev->nplay ()) _alsadev->clear_chan (c++, _fsize);
    // Finish writing to ALSA device.
    _alsadev->play_done (_fsize);
    return _fsize;
}


void Alsathread::thr_main (void)
{
    int     na, nu;
    double  tw, er;

    _alsadev->pcm_start ();
    while (_state != TERM)
    {
        // Wait for next cycle, then take timestamp.
	na = _alsadev->pcm_wait ();  

	tw = tjack (jack_get_time ());
	// Check for errors - requires restart.
	if (_alsadev->state () && (na == 0))
	{
	    _state = WAIT;
	    send (0, 0);
	    usleep (10000);
	    continue;
	}
	
        // Check for commands from the Jack thread.
        if (_commq->rd_avail ())
	{
	    _state = _commq->rd_int32 ();
	    if (_state == PROC) _first = true;
	    if (_state == TERM) send (0, 0);
	}

        // We could have more than one period.
        nu = 0;
        while (na >= _fsize)
       	{
	    // Transfer frames.
	    if (_mode == PLAY) nu += playback ();
	    else               nu += capture ();
            // Update loop condition.
            na -= _fsize;
	    // Run the DLL if in PROC state.
	    if (_state == PROC)
	    {
   	        if (_first)
	        {
		    // Init DLL in first iteration.
		    _first = false;
                    _dt = (double) _fsize / _alsadev->fsamp ();
                    _t0 = tw;
	            _t1 = tw + _dt;
	        } 
	        else 
	        {
		    // Update the DLL.
		    // If we have more than one period, use
                    // the time error only for the last one.
	            if (na >= _fsize) er = 0;
                    else er = tjack_diff (tw, _t1);
	            _t0 = _t1;
	            _t1 = tjack_diff (_t1 + _dt + _w1 * er, 0.0);
	            _dt += _w2 * er;
		}
	    }
	}

	// Send number of frames used and timestamp to Jack thread.
	if (_state == PROC) send (nu, _t1);
    }
    _alsadev->pcm_stop ();
}
