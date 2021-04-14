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


#include <assert.h>
#include "lfqueue.h"


Lfq_adata::Lfq_adata (int size) :
    _size (size),
    _mask (size - 1),
    _nwr (0),
    _nrd (0)
{
    assert (!(_size & _mask));
    _data = new Adata [_size];
}

Lfq_adata::~Lfq_adata (void)
{
    delete[] _data;
} 


Lfq_jdata::Lfq_jdata (int size) :
    _size (size),
    _mask (size - 1),
    _nwr (0),
    _nrd (0)
{
    assert (!(_size & _mask));
    _data = new Jdata [_size];
}

Lfq_jdata::~Lfq_jdata (void)
{
    delete[] _data;
} 


Lfq_int32::Lfq_int32 (int size) :
    _size (size),
    _mask (size - 1),
    _nwr (0),
    _nrd (0)
{
    assert (!(_size & _mask));
    _data = new int32_t [_size];
}

Lfq_int32::~Lfq_int32 (void)
{
    delete[] _data;
} 


Lfq_audio::Lfq_audio (int nsamp, int nchan) :
    _size (nsamp),
    _mask (nsamp - 1),
    _nch (nchan),
    _nwr (0),
    _nrd (0)
{
    assert (!(_size & _mask));
    _data = new float [_nch * _size];
}

Lfq_audio::~Lfq_audio (void)
{
    delete[] _data;
} 


