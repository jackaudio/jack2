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
#else
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#endif

#ifdef __APPLE__
#include <sys/syslimits.h>
#endif

#include <string>
#include <algorithm>
#include <vector>
#include <iostream>
#include <fstream>
#include "jslist.h"
#include "driver_interface.h"

#include "JackExports.h"
#include "JackError.h"

namespace Jack
{

    /*!
    \brief Utility functions.
    */

    struct EXPORT JackTools
    {
        static int GetPID();
        static int GetUID();

        static char* UserDir();
        static char* ServerDir ( const char* server_name, char* server_dir );
        static const char* DefaultServerName();
        static void CleanupFiles ( const char* server_name );
        static int GetTmpdir();
        static void RewriteName ( const char* name, char* new_name );
    };

    /*!
    \brief Internal cient command line parser.
    */

    class EXPORT JackArgParser
    {
        private:

            std::string fArgString;
            int fArgc;
            std::vector<std::string> fArgv;

        public:

            JackArgParser ( const char* arg );
            ~JackArgParser();
            std::string GetArgString();
            int GetNumArgv();
            int GetArgc();
            int GetArgv ( std::vector<std::string>& argv );
            int GetArgv ( char** argv );
            void DeleteArgv ( const char** argv );
            int ParseParams ( jack_driver_desc_t* desc, JSList** param_list );
    };

    /*!
    \brief Generic monitoring class. Saves data to GnuPlot files ('.plt' and '.log' datafile)
    */

    template <class T> class JackGnuPlotMonitor
    {
        private:
            uint32_t fMeasureCnt;
            uint32_t fMeasurePoints;
            T** fMeasureTable;
            uint32_t fTablePos;
            std::string fName;

        public:
            JackGnuPlotMonitor ( uint32_t measure_cnt = 512, uint32_t measure_points = 5, std::string name = std::string ( "default" ) )
            {
                jack_log ( "JackGnuPlotMonitor::JackGnuPlotMonitor measure_cnt %u measure_points %u", measure_cnt, measure_points );

                fMeasureCnt = measure_cnt;
                fMeasurePoints = measure_points;
                fTablePos = 0;
                fName = name;
                fMeasureTable = new T*[fMeasureCnt];
                for ( uint32_t cnt = 0; cnt < fMeasureCnt; cnt++ )
                {
                    fMeasureTable[cnt] = new T[fMeasurePoints];
                    fill_n ( fMeasureTable[cnt], fMeasurePoints, 0 );
                }
            }

            ~JackGnuPlotMonitor()
            {
                jack_log ( "JackGnuPlotMonitor::~JackGnuPlotMonitor" );
                for ( uint32_t cnt = 0; cnt < fMeasureCnt; cnt++ )
                    delete[] fMeasureTable[cnt];
                delete[] fMeasureTable;
            }

            uint32_t Write ( T* measure )
            {
                for ( uint32_t point = 0; point < fMeasurePoints; point++ )
                    fMeasureTable[fTablePos][point] = measure[point];
                if ( ++fTablePos == fMeasureCnt )
                    fTablePos = 0;
                return fTablePos;
            }

            int Save ( std::string name = std::string ( "" ) )
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

            int SetPlotFile ( std::string* options_list = NULL, uint32_t options_number = 0,
                              std::string* field_names = NULL, uint32_t field_number = 0,
                              std::string name = std::string ( "" ) )
            {
                std::string title = ( name.empty() ) ? fName : name;
                std::string plot_filename = title + ".plt";
                std::string data_filename = title + ".log";

                std::ofstream file ( plot_filename.c_str() );

                file << "set multiplot" << std::endl;
                file << "set grid" << std::endl;
                file << "set title \"" << title << "\"" << std::endl;

                for ( uint32_t i = 0; i < options_number; i++ )
                {
                    jack_log ( "JackGnuPlotMonitor::SetPlotFile - Add plot option : '%s'", options_list[i].c_str() );
                    file << options_list[i] << std::endl;
                }

                file << "plot ";
                for ( uint32_t row = 1; row <= field_number; row++ )
                {
                    jack_log ( "JackGnuPlotMonitor::SetPlotFile - Add plot : file '%s' row '%d' title '%s' field '%s'",
                               data_filename.c_str(), row, title.c_str(), field_names[row-1].c_str() );
                    file << "\"" << data_filename << "\" using " << row << " title \"" << field_names[row-1] << "\" with lines";
                    file << ( ( row < field_number ) ? ", " : "\n" );
                }

                jack_log ( "JackGnuPlotMonitor::SetPlotFile - Save GnuPlot '.plt' file to '%s'", plot_filename.c_str() );

                file.close();
                return 0;
            }
    };
}

#endif
