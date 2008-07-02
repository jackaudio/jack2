/*
Copyright (C) 2008 Grame

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

#ifndef __JackBlockingNetIOAdapter__
#define __JackBlockingNetIOAdapter__

#include "JackNetIOAdapter.h"

namespace Jack
{

	class JackBlockingNetIOAdapter : public JackNetIOAdapter
	{
		private:
      	
            static int Process(jack_nframes_t, void* arg) {return 0;}
            
		public:
        
			JackBlockingNetIOAdapter(jack_client_t* jack_client, 
                                JackIOAdapterInterface* audio_io, 
                                int input, 
                                int output)
                                : JackNetIOAdapter(jack_client, audio_io, input, output)
			~JackBlockingNetIOAdapter()
            {}
            
 	};
    
}

#endif
