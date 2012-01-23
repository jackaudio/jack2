/*
  Copyright (C) 2006-2008 Grame

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

#ifndef __JackGnuPlotMonitor__
#define __JackGnuPlotMonitor__

#include <stdlib.h>
#include <jack/systemdeps.h>

#include <string>
#include <algorithm>
#include <vector>
#include <iostream>
#include <fstream>

namespace Jack
{

    /*!
    \brief Generic monitoring class. Saves data to GnuPlot files ('.plt' and '.log' datafile)

    This template class allows to manipulate monitoring records, and automatically generate the GnuPlot config and data files.
    Operations are RT safe because it uses fixed size data buffers.
    You can set the number of measure points, and the number of records.

    To use it :
    - create a JackGnuPlotMonitor, you can use the data type you want.
    - create a temporary array for your measure
    - once you have filled this array with 'measure points' value, call write() to add it to the record
    - once you've done with your measurment, just call save() to save your data file

    You can also call SetPlotFile() to automatically generate '.plt' file from an options list.

    */

    template <class T> class JackGnuPlotMonitor
    {
        private:
            uint32_t fMeasureCnt;
            uint32_t fMeasurePoints;
            uint32_t fMeasureId;
            T* fCurrentMeasure;
            T** fMeasureTable;
            uint32_t fTablePos;
            std::string fName;

        public:

            JackGnuPlotMonitor(uint32_t measure_cnt = 512, uint32_t measure_points = 5, std::string name = std::string("default"));

            ~JackGnuPlotMonitor();

            T AddNew(T measure_point);

			uint32_t New();

            T Add(T measure_point);

            uint32_t AddLast(T measure_point);

            uint32_t Write();

            int Save(std::string name = std::string(""));

            int SetPlotFile(std::string* options_list = NULL, uint32_t options_number = 0,
                            std::string* field_names = NULL, uint32_t field_number = 0,
                            std::string name = std::string(""));
    };

}

#endif

