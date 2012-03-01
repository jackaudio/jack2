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

#include "JackGnuPlotMonitor.h"
#include "JackError.h"

using namespace std;

namespace Jack {

template <class T>
JackGnuPlotMonitor<T>::JackGnuPlotMonitor(uint32_t measure_cnt, uint32_t measure_points, std::string name)
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
        fill_n ( fMeasureTable[cnt], fMeasurePoints, 0 );
    }
}

template <class T>
JackGnuPlotMonitor<T>::~JackGnuPlotMonitor()
{
    jack_log ( "JackGnuPlotMonitor::~JackGnuPlotMonitor" );

    for ( uint32_t cnt = 0; cnt < fMeasureCnt; cnt++ )
        delete[] fMeasureTable[cnt];
    delete[] fMeasureTable;
    delete[] fCurrentMeasure;
}

template <class T>
T JackGnuPlotMonitor<T>::AddNew(T measure_point)
{
    fMeasureId = 0;
    return fCurrentMeasure[fMeasureId++] = measure_point;
}

template <class T>
uint32_t JackGnuPlotMonitor<T>::New()
{
    return fMeasureId = 0;
}

template <class T>
T JackGnuPlotMonitor<T>::Add(T measure_point)
{
    return fCurrentMeasure[fMeasureId++] = measure_point;
}

template <class T>
uint32_t JackGnuPlotMonitor<T>::AddLast(T measure_point)
{
    fCurrentMeasure[fMeasureId] = measure_point;
    fMeasureId = 0;
    return Write();
}

template <class T>
uint32_t JackGnuPlotMonitor<T>::Write()
{
    for ( uint32_t point = 0; point < fMeasurePoints; point++ )
        fMeasureTable[fTablePos][point] = fCurrentMeasure[point];
    if ( ++fTablePos == fMeasureCnt )
        fTablePos = 0;
    return fTablePos;
}

template <class T>
int JackGnuPlotMonitor<T>::Save(std::string name)
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

template <class T>
int JackGnuPlotMonitor<T>::SetPlotFile(std::string* options_list, uint32_t options_number,
                std::string* field_names, uint32_t field_number,
                std::string name)
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

}  // end of namespace


