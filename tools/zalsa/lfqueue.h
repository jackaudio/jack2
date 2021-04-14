// ----------------------------------------------------------------------------
//
//  Copyright (C) 2012 Fons Adriaensen <fons@linuxaudio.org>
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


#ifndef __LFQUEUE_H
#define __LFQUEUE_H


#include <stdint.h>
#include <string.h>


class Adata
{
public:

    int32_t  _state;
    int32_t  _nsamp;
    double   _timer;
};


class Lfq_adata
{
public:

    Lfq_adata (int size);
    ~Lfq_adata (void); 

    void reset (void) { _nwr = _nrd = 0; }
    int  size (void) const { return _size; }

    int       wr_avail (void) const { return _size - _nwr + _nrd; } 
    Adata    *wr_datap (void) { return _data + (_nwr & _mask); }
    void      wr_commit (void) { _nwr++; }

    int       rd_avail (void) const { return _nwr - _nrd; } 
    Adata    *rd_datap (void) { return _data + (_nrd & _mask); }
    void      rd_commit (void) { _nrd++; }

private:

    Adata    *_data;
    int       _size;
    int       _mask;
    int       _nwr;
    int       _nrd;
};


class Jdata
{
public:

    int32_t  _state;
    double   _error;
    double   _ratio;
    int      _bstat;
};


class Lfq_jdata
{
public:

    Lfq_jdata (int size);
    ~Lfq_jdata (void); 

    void reset (void) { _nwr = _nrd = 0; }
    int  size (void) const { return _size; }

    int       wr_avail (void) const { return _size - _nwr + _nrd; } 
    Jdata    *wr_datap (void) { return _data + (_nwr & _mask); }
    void      wr_commit (void) { _nwr++; }

    int       rd_avail (void) const { return _nwr - _nrd; } 
    Jdata    *rd_datap (void) { return _data + (_nrd & _mask); }
    void      rd_commit (void) { _nrd++; }

private:

    Jdata    *_data;
    int       _size;
    int       _mask;
    int       _nwr;
    int       _nrd;
};


class Lfq_int32
{
public:

    Lfq_int32 (int size);
    ~Lfq_int32 (void); 

    int  size (void) const { return _size; }
    void reset (void) { _nwr = _nrd = 0; }

    int      wr_avail (void) const { return _size - _nwr + _nrd; } 
    int32_t *wr_datap (void) { return _data + (_nwr & _mask); }
    void     wr_commit (void) { _nwr++; }

    int      rd_avail (void) const { return _nwr - _nrd; } 
    int32_t *rd_datap (void) { return _data + (_nrd & _mask); }
    void     rd_commit (void) { _nrd++; }

    void     wr_int32 (int32_t v) { _data [_nwr++ & _mask] = v; }
    void     wr_uint32 (uint32_t v) { _data [_nwr++ & _mask] = v; }
    void     wr_float (float v) { *(float *)(_data + (_nwr++ & _mask)) = v; }

    int32_t  rd_int32 (void) { return  _data [_nrd++ & _mask]; }
    int32_t  rd_uint32 (void) { return _data [_nrd++ & _mask]; }
    float    rd_float (void) { return *(float *)(_data + (_nrd++ & _mask)); }

private:

    int32_t  *_data;
    int       _size;
    int       _mask;
    int       _nwr;
    int       _nrd;
};


class Lfq_audio
{
public:

    Lfq_audio (int nsamp, int nchan);
    ~Lfq_audio (void); 

    int  size (void) const { return _size; }
    void reset (void)
    {
        _nwr = _nrd = 0;
	memset (_data, 0, _size * _nch * sizeof (float));
    }

    int     nchan (void) const { return _nch; } 
    int     nwr (void) const { return _nwr; };
    int     nrd (void) const { return _nrd; };

    int     wr_avail (void) const { return _size - _nwr + _nrd; } 
    int     wr_linav (void) const { return _size - (_nwr & _mask); }
    float  *wr_datap (void) { return _data + _nch * (_nwr & _mask); }
    void    wr_commit (int k) { _nwr += k; }

    int     rd_avail (void) const { return _nwr - _nrd; } 
    int     rd_linav (void) const { return _size - (_nrd & _mask); }
    float  *rd_datap (void) { return _data + _nch * (_nrd & _mask); }
    void    rd_commit (int k) { _nrd += k; }

private:

    float    *_data;
    int       _size;
    int       _mask;
    int       _nch;
    int       _nwr;
    int       _nrd;
};


#endif

