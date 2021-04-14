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


#ifndef __JACKCLIENT_H
#define __JACKCLIENT_H


#include <zita-resampler/vresampler.h>
#include "jack/jack.h"
#include "lfqueue.h"


class Jackclient
{
public:

    Jackclient (jack_client_t*, const char *jserv, int mode, int nchan, bool sync, void *arg);
    virtual ~Jackclient (void);
    
    enum { PLAY, CAPT, MAXCHAN = 64 };
    enum { INIT, TERM, WAIT, SYNC0, SYNC1, SYNC2, PROC1, PROC2 };

    void start (Lfq_audio   *audioq,
                Lfq_int32   *commq, 
	        Lfq_adata   *alsaq,
		Lfq_jdata   *infoq,
                double      ratio,
	        int         delay,
	        int         ltcor,
	        int         rqual);

    const char *jname (void) const { return _jname; }
    int fsamp (void) const { return _fsamp; }
    int bsize (void) const { return _bsize; }
    int rprio (void) const { return _rprio; }
    void *getarg(void) const { return _arg; }

private:

    bool init (const char *jserv);
    void fini (void);
    void initwait (int nwait);
    void initsync (void);
    void setloop (double bw);
    void silence (int nframes);
    void playback (int nframes);
    void capture (int nframes);
    void sendinfo (int state, double error, double ratio);

    virtual void thr_main (void) {}

    void jack_freewheel (int state);
    void jack_latency (jack_latency_callback_mode_t jlcm);
    int  jack_process (int nframes);

    jack_client_t  *_client;
    jack_port_t    *_ports [MAXCHAN];
    void           *_arg;
    const char     *_jname;
    int             _mode;
    int             _nchan;
    int             _state;
    int             _count;
    int             _fsamp;
    int             _bsize;
    int             _rprio;
    bool            _freew;
    float          *_buff;
 
    Lfq_audio      *_audioq;
    Lfq_int32      *_commq; 
    Lfq_adata      *_alsaq;
    Lfq_jdata      *_infoq;
    double          _ratio;
    int             _ppsec;
    int             _bstat;

    jack_nframes_t  _ft;
    double          _t_a0;
    double          _t_a1;
    int             _k_a0;
    int             _k_a1;
    double          _delay;
    int             _ltcor;

    double           _w0;
    double           _w1;
    double           _w2;
    double           _z1;
    double           _z2;
    double           _z3;
    double          _rcorr;
    VResampler     *_resamp;

    static void jack_static_shutdown (void *arg);
    static int  jack_static_buffsize (jack_nframes_t nframes, void *arg);
    static void jack_static_freewheel (int state, void *arg);
    static void jack_static_latency (jack_latency_callback_mode_t jlcm, void *arg);
    static int  jack_static_process (jack_nframes_t nframes, void *arg);
};


#endif
