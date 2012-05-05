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

#ifndef __JackTools__
#define __JackTools__

#ifdef WIN32
#include <windows.h>
#define DIR_SEPARATOR '\\'
#else
#define DIR_SEPARATOR '/'
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#endif

#ifdef __APPLE__
#include <sys/syslimits.h>
#endif

#include "jslist.h"
#include "JackCompilerDeps.h"
#include "JackError.h"

#include <string>
#include <algorithm>
#include <vector>
#include <iostream>
#include <fstream>

namespace Jack
{

    /*!
    \brief Utility functions.
    */

    struct SERVER_EXPORT JackTools
    {
        static int GetPID();
        static int GetUID();

        static void KillServer();

        static int MkDir(const char* path);
        static char* UserDir();
        static char* ServerDir(const char* server_name, char* server_dir);
        static const char* DefaultServerName();
        static void CleanupFiles(const char* server_name);
        static int GetTmpdir();
        static void RewriteName(const char* name, char* new_name);
        static void ThrowJackNetException();

        // For OSX only
        static int ComputationMicroSec(int buffer_size)
        {
            if (buffer_size < 128) {
                return 500;
            } else if (buffer_size < 256) {
                return 300;
            } else {
                return 100;
            }
        }
    };

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
	    JackGnuPlotMonitor(uint32_t measure_cnt, uint32_t measure_points, std::string name)
	    {
		    jack_log ( "JackGnuPlotMonitor::JackGnuPlotMonitor %u measure points - %u measures", measure_points, measure_cnt );

		    fMeasureCnt = measure_cnt;
		    fMeasurePoints = measure_points;
		    fTablePos = 0;
		    fName = name;
		    fCurrentMeasure = new T[fMeasurePoints];
		    fMeasureTable = new T*[fMeasureCnt];
		    for ( uint32_t cnt = 0; cnt < fMeasureCnt; cnt++ )
		    {
			    fMeasureTable[cnt] = new T[fMeasurePoints];
			    std::fill_n ( fMeasureTable[cnt], fMeasurePoints, 0 );
		    }
	    }

	    ~JackGnuPlotMonitor()
	    {
		    jack_log ( "JackGnuPlotMonitor::~JackGnuPlotMonitor" );

		    for ( uint32_t cnt = 0; cnt < fMeasureCnt; cnt++ )
			    delete[] fMeasureTable[cnt];
		    delete[] fMeasureTable;
		    delete[] fCurrentMeasure;
	    }

	    T AddNew(T measure_point)
	    {
		    fMeasureId = 0;
		    return fCurrentMeasure[fMeasureId++] = measure_point;
	    }

	    uint32_t New()
	    {
		    return fMeasureId = 0;
	    }

	    T Add(T measure_point)
	    {
		    return fCurrentMeasure[fMeasureId++] = measure_point;
	    }

	    uint32_t AddLast(T measure_point)
	    {
		    fCurrentMeasure[fMeasureId] = measure_point;
		    fMeasureId = 0;
		    return Write();
	    }

	    uint32_t Write()
	    {
		    for ( uint32_t point = 0; point < fMeasurePoints; point++ )
			    fMeasureTable[fTablePos][point] = fCurrentMeasure[point];
		    if ( ++fTablePos == fMeasureCnt )
			    fTablePos = 0;
		    return fTablePos;
	    }

	    int Save(std::string name = std::string ( "" ))
	    {
		    std::string filename = ( name.empty() ) ? fName : name;
		    filename += ".log";

		    jack_log ( "JackGnuPlotMonitor::Save filename %s", filename.c_str() );

		    std::ofstream file ( filename.c_str() );

		    for ( uint32_t cnt = 0; cnt < fMeasureCnt; cnt++ )
		    {
			    for ( uint32_t point = 0; point < fMeasurePoints; point++ )
				    file << fMeasureTable[cnt][point] << " \t";
			    file << std::endl;
		    }

		    file.close();
		    return 0;
	    }

	    int SetPlotFile(std::string* options_list, uint32_t options_number,
			    std::string* field_names, uint32_t field_number,
			    std::string name = std::string ( "" ))
	    {
		    std::string title = ( name.empty() ) ? fName : name;
		    std::string plot_filename = title + ".plt";
		    std::string data_filename = title + ".log";

		    std::ofstream file ( plot_filename.c_str() );

		    file << "set multiplot" << std::endl;
		    file << "set grid" << std::endl;
		    file << "set title \"" << title << "\"" << std::endl;

		    for ( uint32_t i = 0; i < options_number; i++ )
			    file << options_list[i] << std::endl;

		    file << "plot ";
		    for ( uint32_t row = 1; row <= field_number; row++ )
		    {
			    file << "\"" << data_filename << "\" using " << row << " title \"" << field_names[row-1] << "\" with lines";
			    file << ( ( row < field_number ) ? ", " : "\n" );
		    }

		    jack_log ( "JackGnuPlotMonitor::SetPlotFile - Save GnuPlot file to '%s'", plot_filename.c_str() );

		    file.close();
		    return 0;
	    }

    };

    void BuildClientPath(char* path_to_so, int path_len, const char* so_name);
    void PrintLoadError(const char* so_name);

}

#endif
