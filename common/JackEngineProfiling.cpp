/*
Copyright (C) 2008 Grame & RTL

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

#include "JackEngineProfiling.h"
#include "JackGraphManager.h"
#include "JackClientControl.h"
#include "JackEngineControl.h"
#include "JackClientInterface.h"
#include "JackGlobals.h"
#include "JackTime.h"

namespace Jack
{

JackEngineProfiling::JackEngineProfiling():fAudioCycle(0),fMeasuredClient(0)
{
    jack_info("Engine profiling activated, beware %ld MBytes are needed to record profiling points...", sizeof(fProfileTable) / (1024 * 1024));
    
    // Force memory page in
    memset(fProfileTable, 0, sizeof(fProfileTable));
}

JackEngineProfiling::~JackEngineProfiling()
{
    FILE* file = fopen("JackEngineProfiling.log", "w");
    char buffer[1024];
    
    jack_info("Write server and clients timing data...");

    if (file == NULL) {
        jack_error("JackEngineProfiling::Save cannot open JackEngineProfiling.log file");
    } else {
    
        // For each measured point
        for (int i = 2; i < TIME_POINTS; i++) {
            
            // Driver timing values
            long d1 = long(fProfileTable[i].fCurCycleBegin - fProfileTable[i - 1].fCurCycleBegin);
            long d2 = long(fProfileTable[i].fPrevCycleEnd - fProfileTable[i - 1].fCurCycleBegin);
            
            if (d1 <= 0 || fProfileTable[i].fAudioCycle <= 0)
                continue; // Skip non valid cycles
                
            // Print driver delta and end cycle
            fprintf(file, "%ld \t %ld \t", d1, d2);
             
            // For each measured client
            for (unsigned int j = 0; j < fMeasuredClient; j++) { 
            
                int ref = fIntervalTable[j].fRefNum;
            
                // Is valid client cycle 
                 if (fProfileTable[i].fClientTable[ref].fStatus != NotTriggered) {
             
                    long d5 = long(fProfileTable[i].fClientTable[ref].fSignaledAt - fProfileTable[i - 1].fCurCycleBegin);
                    long d6 = long(fProfileTable[i].fClientTable[ref].fAwakeAt - fProfileTable[i - 1].fCurCycleBegin);
                    long d7 = long(fProfileTable[i].fClientTable[ref].fFinishedAt - fProfileTable[i - 1].fCurCycleBegin);
             
                    // Print ref, signal, start, end, scheduling, duration, status
                    fprintf(file, "%d \t %ld \t %ld \t %ld \t %ld \t %ld \t %d \t", 
                            ref,
                            ((d5 > 0) ? d5 : 0),
                            ((d6 > 0) ? d6 : 0),
                            ((d7 > 0) ? d7 : 0),
                            ((d6 > 0 && d5 > 0) ? (d6 - d5) : 0),
                            ((d7 > 0 && d6 > 0) ? (d7 - d6) : 0),
                            fProfileTable[i].fClientTable[ref].fStatus);
                } else { // Print tabs
                     fprintf(file, "\t  \t  \t  \t  \t  \t \t");
                }
            }
            
            // Terminate line
            fprintf(file, "\n");
        }
    }
    
    // Driver period
    file = fopen("Timing1.plot", "w");

    if (file == NULL) {
        jack_error("JackEngineProfiling::Save cannot open Timing1.log file");
    } else {
        
        fprintf(file, "set grid\n");
        fprintf(file, "set title \"Audio driver timing\"\n");
        fprintf(file, "set xlabel \"audio cycles\"\n");
        fprintf(file, "set ylabel \"usec\"\n");
        fprintf(file, "plot \"JackEngineProfiling.log\" using 1 title \"Audio period\" with lines \n");
    
        fprintf(file, "set output 'Timing1.pdf\n");
        fprintf(file, "set terminal pdf\n");
    
        fprintf(file, "set grid\n");
        fprintf(file, "set title \"Audio driver timing\"\n");
        fprintf(file, "set xlabel \"audio cycles\"\n");
        fprintf(file, "set ylabel \"usec\"\n");
        fprintf(file, "plot \"JackEngineProfiling.log\" using 1 title \"Audio period\" with lines \n");
    
        fclose(file);
    }
    
    // Driver end date
    file = fopen("Timing2.plot", "w");

    if (file == NULL) {
        jack_error("JackEngineProfiling::Save cannot open Timing2.log file");
    } else {
   
        fprintf(file, "set grid\n");
        fprintf(file, "set title \"Driver end date\"\n");
        fprintf(file, "set xlabel \"audio cycles\"\n");
        fprintf(file, "set ylabel \"usec\"\n");
        fprintf(file, "plot  \"JackEngineProfiling.log\" using 2 title \"Driver end date\" with lines \n");
    
        fprintf(file, "set output 'Timing2.pdf\n");
        fprintf(file, "set terminal pdf\n");
    
        fprintf(file, "set grid\n");
        fprintf(file, "set title \"Driver end date\"\n");
        fprintf(file, "set xlabel \"audio cycles\"\n");
        fprintf(file, "set ylabel \"usec\"\n");
        fprintf(file, "plot  \"JackEngineProfiling.log\" using 2 title \"Driver end date\" with lines \n");
    
        fclose(file);
    }
        
    // Clients end date
    if (fMeasuredClient > 0) {
        file = fopen("Timing3.plot", "w");
        if (file == NULL) {
            jack_error("JackEngineProfiling::Save cannot open Timing3.log file");
        } else {
        
            fprintf(file, "set multiplot\n");
            fprintf(file, "set grid\n");
            fprintf(file, "set title \"Clients end date\"\n");
            fprintf(file, "set xlabel \"audio cycles\"\n");
            fprintf(file, "set ylabel \"usec\"\n");
            fprintf(file, "plot ");
            for (unsigned int i = 0; i < fMeasuredClient; i++) {
                if (i == 0) {
                    if (i + 1 == fMeasuredClient) { // Last client
                        sprintf(buffer, "\"JackEngineProfiling.log\" using 1 title \"Audio period\" with lines,\"JackEngineProfiling.log\" using %d title \"%s\" with lines", 
                        ((i + 1) * 7) - 1 , fIntervalTable[i].fName);
                    } else {
                        sprintf(buffer, "\"JackEngineProfiling.log\" using 1 title \"Audio period\" with lines,\"JackEngineProfiling.log\" using %d title \"%s\" with lines,", 
                        ((i + 1) * 7) - 1 , fIntervalTable[i].fName);
                    }
                } else if (i + 1 == fMeasuredClient) { // Last client
                    sprintf(buffer, "\"JackEngineProfiling.log\" using %d title \"%s\" with lines", ((i + 1) * 7) - 1 , fIntervalTable[i].fName);
                } else {
                    sprintf(buffer, "\"JackEngineProfiling.log\" using %d title \"%s\" with lines,", ((i + 1) * 7) - 1, fIntervalTable[i].fName);
                }
                fprintf(file, buffer);
            }
        
            fprintf(file, "\n unset multiplot\n");  
            fprintf(file, "set output 'Timing3.pdf\n");
            fprintf(file, "set terminal pdf\n");
        
            fprintf(file, "set multiplot\n");
            fprintf(file, "set grid\n");
            fprintf(file, "set title \"Clients end date\"\n");
            fprintf(file, "set xlabel \"audio cycles\"\n");
            fprintf(file, "set ylabel \"usec\"\n");
            fprintf(file, "plot ");
            for (unsigned int i = 0; i < fMeasuredClient; i++) {
                if (i == 0) {
                    if ((i + 1) == fMeasuredClient) { // Last client
                        sprintf(buffer, "\"JackEngineProfiling.log\" using 1 title \"Audio period\" with lines,\"JackEngineProfiling.log\" using %d title \"%s\" with lines", 
                        ((i + 1) * 7) - 1 , fIntervalTable[i].fName);
                    } else {
                        sprintf(buffer, "\"JackEngineProfiling.log\" using 1 title \"Audio period\" with lines,\"JackEngineProfiling.log\" using %d title \"%s\" with lines,", 
                        ((i + 1) * 7) - 1 , fIntervalTable[i].fName);
                    }
                } else if ((i + 1) == fMeasuredClient) { // Last client
                    sprintf(buffer, "\"JackEngineProfiling.log\" using %d title \"%s\" with lines", ((i + 1) * 7) - 1 , fIntervalTable[i].fName);
                } else {
                    sprintf(buffer, "\"JackEngineProfiling.log\" using %d title \"%s\" with lines,", ((i + 1) * 7) - 1, fIntervalTable[i].fName);
                }
                fprintf(file, buffer);
            }
             
            fclose(file);
        }
    }

    // Clients scheduling
    if (fMeasuredClient > 0) {
        file = fopen("Timing4.plot", "w");

        if (file == NULL) {
            jack_error("JackEngineProfiling::Save cannot open Timing4.log file");
        } else {
        
            fprintf(file, "set multiplot\n");
            fprintf(file, "set grid\n");
            fprintf(file, "set title \"Clients scheduling latency\"\n");
            fprintf(file, "set xlabel \"audio cycles\"\n");
            fprintf(file, "set ylabel \"usec\"\n");
            fprintf(file, "plot ");
            for (unsigned int i = 0; i < fMeasuredClient; i++) {
                if ((i + 1) == fMeasuredClient) // Last client
                    sprintf(buffer, "\"JackEngineProfiling.log\" using %d title \"%s\" with lines", ((i + 1) * 7), fIntervalTable[i].fName);
                else
                    sprintf(buffer, "\"JackEngineProfiling.log\" using %d title \"%s\" with lines,", ((i + 1) * 7), fIntervalTable[i].fName);
                fprintf(file, buffer);
            }
            
            fprintf(file, "\n unset multiplot\n");  
            fprintf(file, "set output 'Timing4.pdf\n");
            fprintf(file, "set terminal pdf\n");
            
            fprintf(file, "set multiplot\n");
            fprintf(file, "set grid\n");
            fprintf(file, "set title \"Clients scheduling latency\"\n");
            fprintf(file, "set xlabel \"audio cycles\"\n");
            fprintf(file, "set ylabel \"usec\"\n");
            fprintf(file, "plot ");
            for (unsigned int i = 0; i < fMeasuredClient; i++) {
                if ((i + 1) == fMeasuredClient) // Last client
                    sprintf(buffer, "\"JackEngineProfiling.log\" using %d title \"%s\" with lines", ((i + 1) * 7), fIntervalTable[i].fName);
                else
                    sprintf(buffer, "\"JackEngineProfiling.log\" using %d title \"%s\" with lines,", ((i + 1) * 7), fIntervalTable[i].fName);
                fprintf(file, buffer);
            }
            fclose(file);
        }
    }
    
     // Clients duration
    if (fMeasuredClient > 0) {
        file = fopen("Timing5.plot", "w");

        if (file == NULL) {
            jack_error("JackEngineProfiling::Save cannot open Timing5.log file");
        } else {
        
            fprintf(file, "set multiplot\n");
            fprintf(file, "set grid\n");
            fprintf(file, "set title \"Clients duration\"\n");
            fprintf(file, "set xlabel \"audio cycles\"\n");
            fprintf(file, "set ylabel \"usec\"\n");
            fprintf(file, "plot ");
            for (unsigned int i = 0; i < fMeasuredClient; i++) {
                if ((i + 1) == fMeasuredClient) // Last client
                    sprintf(buffer, "\"JackEngineProfiling.log\" using %d title \"%s\" with lines", ((i + 1) * 7) + 1, fIntervalTable[i].fName);
                else
                    sprintf(buffer, "\"JackEngineProfiling.log\" using %d title \"%s\" with lines,", ((i + 1) * 7) + 1, fIntervalTable[i].fName);
                fprintf(file, buffer);
            }
            
            fprintf(file, "\n unset multiplot\n");  
            fprintf(file, "set output 'Timing5.pdf\n");
            fprintf(file, "set terminal pdf\n");
            
            fprintf(file, "set multiplot\n");
            fprintf(file, "set grid\n");
            fprintf(file, "set title \"Clients duration\"\n");
            fprintf(file, "set xlabel \"audio cycles\"\n");
            fprintf(file, "set ylabel \"usec\"\n");
            fprintf(file, "plot ");
            for (unsigned int i = 0; i < fMeasuredClient; i++) {
                if ((i + 1) == fMeasuredClient) // Last client
                    sprintf(buffer, "\"JackEngineProfiling.log\" using %d title \"%s\" with lines", ((i + 1) * 7) + 1, fIntervalTable[i].fName);
                else
                    sprintf(buffer, "\"JackEngineProfiling.log\" using %d title \"%s\" with lines,", ((i + 1) * 7) + 1, fIntervalTable[i].fName);
                fprintf(file, buffer);
            }
            fclose(file);
        }
    }
}

bool JackEngineProfiling::CheckClient(const char* name, int cur_point)
{
    for (int i = 0; i < MEASURED_CLIENTS; i++) {
       if (strcmp(fIntervalTable[i].fName, name) == 0) {
            fIntervalTable[i].fEndInterval = cur_point;
            return true;
        }
    }
    return false;
}

void JackEngineProfiling::Profile(JackClientInterface** table, 
                                   JackGraphManager* manager, 
                                   jack_time_t period_usecs,
                                   jack_time_t cur_cycle_begin, 
                                   jack_time_t prev_cycle_end)
{
    fAudioCycle = (fAudioCycle + 1) % TIME_POINTS;
  
    // Keeps cycle data
    fProfileTable[fAudioCycle].fPeriodUsecs = period_usecs;
    fProfileTable[fAudioCycle].fCurCycleBegin = cur_cycle_begin;
    fProfileTable[fAudioCycle].fPrevCycleEnd = prev_cycle_end;
    fProfileTable[fAudioCycle].fAudioCycle = fAudioCycle;

    for (int i = GetEngineControl()->fDriverNum; i < CLIENT_NUM; i++) {
        JackClientInterface* client = table[i];
        JackClientTiming* timing = manager->GetClientTiming(i);
        if (client && client->GetClientControl()->fActive && client->GetClientControl()->fCallback[kRealTimeCallback]) {
           
            if (!CheckClient(client->GetClientControl()->fName, fAudioCycle)) {
                // Keep new measured client
                fIntervalTable[fMeasuredClient].fRefNum = i;
                strcpy(fIntervalTable[fMeasuredClient].fName, client->GetClientControl()->fName);
                fIntervalTable[fMeasuredClient].fBeginInterval = fAudioCycle;
                fIntervalTable[fMeasuredClient].fEndInterval = fAudioCycle;
                fMeasuredClient++;
            }
            fProfileTable[fAudioCycle].fClientTable[i].fRefNum = i;
            fProfileTable[fAudioCycle].fClientTable[i].fSignaledAt = timing->fSignaledAt;
            fProfileTable[fAudioCycle].fClientTable[i].fAwakeAt = timing->fAwakeAt;
            fProfileTable[fAudioCycle].fClientTable[i].fFinishedAt = timing->fFinishedAt;
            fProfileTable[fAudioCycle].fClientTable[i].fStatus = timing->fStatus;
        }
    }
}

JackTimingMeasure* JackEngineProfiling::GetCurMeasure()
{
    return &fProfileTable[fAudioCycle];
}
    
} // end of namespace
