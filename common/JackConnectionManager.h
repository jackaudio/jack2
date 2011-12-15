/*
Copyright (C) 2004-2008 Grame

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

#ifndef __JackConnectionManager__
#define __JackConnectionManager__

#include "JackConstants.h"
#include "JackActivationCount.h"
#include "JackError.h"
#include "JackCompilerDeps.h"
#include <vector>
#include <assert.h>

namespace Jack
{

struct JackClientControl;

/*!
\brief Utility class.
*/

PRE_PACKED_STRUCTURE
template <int SIZE>
class JackFixedArray
{

    private:

        jack_int_t fTable[SIZE];
        uint32_t fCounter;

    public:

        JackFixedArray()
        {
            Init();
        }

        void Init()
        {
            for (int i = 0; i < SIZE; i++)
                fTable[i] = EMPTY;
            fCounter = 0;
        }

        bool AddItem(jack_int_t index)
        {
            for (int i = 0; i < SIZE; i++) {
                if (fTable[i] == EMPTY) {
                    fTable[i] = index;
                    fCounter++;
                    return true;
                }
            }
            return false;
        }

        bool RemoveItem(jack_int_t index)
        {
            for (int i = 0; i < SIZE; i++) {
                if (fTable[i] == index) {
                    fCounter--;
                    // Shift all indexes
                    if (i == SIZE - 1) {
                        fTable[i] = EMPTY;
                    } else {
                        int j;
                        for (j = i; j <= SIZE - 2 && fTable[j] != EMPTY; j++) {
                            fTable[j] = fTable[j + 1];
                        }
                        fTable[j] = EMPTY;
                    }
                    return true;
                }
            }
            return false;
        }

        jack_int_t GetItem(jack_int_t index) const
        {
            return (index < SIZE) ? fTable[index] : EMPTY;
        }

        const jack_int_t* GetItems() const
        {
            return fTable;
        }

        bool CheckItem(jack_int_t index) const
        {
            for (int i = 0; i < SIZE && fTable[i] != EMPTY; i++) {
                if (fTable[i] == index)
                    return true;
            }
            return false;
        }

        uint32_t GetItemCount() const
        {
            return fCounter;
        }

} POST_PACKED_STRUCTURE;

/*!
\brief Utility class.
*/

PRE_PACKED_STRUCTURE
template <int SIZE>
class JackFixedArray1 : public JackFixedArray<SIZE>
{
    private:

        bool fUsed;

    public:

        JackFixedArray1()
        {
            Init();
        }

        void Init()
        {
            JackFixedArray<SIZE>::Init();
            fUsed = false;
        }

        bool IsAvailable()
        {
            if (fUsed) {
                return false;
            } else {
                fUsed = true;
                return true;
            }
        }

} POST_PACKED_STRUCTURE;

/*!
\brief Utility class.
*/

PRE_PACKED_STRUCTURE
template <int SIZE>
class JackFixedMatrix
{
    private:

        jack_int_t fTable[SIZE][SIZE];

    public:

        JackFixedMatrix()
        {}

        void Init(jack_int_t index)
        {
            for (int i = 0; i < SIZE; i++) {
                fTable[index][i] = 0;
                fTable[i][index] = 0;
            }
        }

        const jack_int_t* GetItems(jack_int_t index) const
        {
            return fTable[index];
        }

        jack_int_t IncItem(jack_int_t index1, jack_int_t index2)
        {
            fTable[index1][index2]++;
            return fTable[index1][index2];
        }

        jack_int_t DecItem(jack_int_t index1, jack_int_t index2)
        {
            fTable[index1][index2]--;
            return fTable[index1][index2];
        }

        jack_int_t GetItemCount(jack_int_t index1, jack_int_t index2) const
        {
            return fTable[index1][index2];
        }

        void ClearItem(jack_int_t index1, jack_int_t index2)
        {
            fTable[index1][index2] = 0;
        }

        /*!
        	\brief Get the output indexes of a given index.
        */
        void GetOutputTable(jack_int_t index, jack_int_t* output) const
        {
            int i, j;

            for (i = 0; i < SIZE; i++)
                output[i] = EMPTY;

            for (i = 0, j = 0; i < SIZE; i++) {
                if (fTable[index][i] > 0) {
                    output[j] = i;
                    j++;
                }
            }
        }

        void GetOutputTable1(jack_int_t index, jack_int_t* output) const
        {
            for (int i = 0; i < SIZE; i++) {
                output[i] = fTable[i][index];
            }
        }

        bool IsInsideTable(jack_int_t index, jack_int_t* output) const
        {
            for (int i = 0; i < SIZE && output[i] != EMPTY; i++) {
                if (output[i] == index)
                    return true;
            }
            return false;
        }

        void Copy(JackFixedMatrix& copy)
        {
            for (int i = 0; i < SIZE; i++) {
                memcpy(copy.fTable[i], fTable[i], sizeof(jack_int_t) * SIZE);
            }
        }


} POST_PACKED_STRUCTURE;

/*!
\brief Utility class.
*/

PRE_PACKED_STRUCTURE
template <int SIZE>
class JackLoopFeedback
{
    private:

        int fTable[SIZE][3];

        /*!
        	\brief Add a feedback connection between 2 refnum.
        */
        bool AddConnectionAux(int ref1, int ref2)
        {
            for (int i = 0; i < SIZE; i++) {
                if (fTable[i][0] == EMPTY) {
                    fTable[i][0] = ref1;
                    fTable[i][1] = ref2;
                    fTable[i][2] = 1;
                    jack_log("JackLoopFeedback::AddConnectionAux ref1 = %ld ref2 = %ld", ref1, ref2);
                    return true;
                }
            }
            jack_error("Feedback table is full !!\n");
            return false;
        }

        /*!
        	\brief Remove a feedback connection between 2 refnum.
        */
        bool RemoveConnectionAux(int ref1, int ref2)
        {
            for (int i = 0; i < SIZE; i++) {
                if (fTable[i][0] == ref1 && fTable[i][1] == ref2) {
                    fTable[i][0] = EMPTY;
                    fTable[i][1] = EMPTY;
                    fTable[i][2] = 0;
                    jack_log("JackLoopFeedback::RemoveConnectionAux ref1 = %ld ref2 = %ld", ref1, ref2);
                    return true;
                }
            }
            jack_error("Feedback connection not found\n");
            return false;
        }

        int IncConnection(int index)
        {
            fTable[index][2]++;
            return fTable[index][2];
        }

        int DecConnection(int index)
        {
            fTable[index][2]--;
            return fTable[index][2];
        }

    public:

        JackLoopFeedback()
        {
            Init();
        }

        void Init()
        {
            for (int i = 0; i < SIZE; i++) {
                fTable[i][0] = EMPTY;
                fTable[i][1] = EMPTY;
                fTable[i][2] = 0;
            }
        }

        bool IncConnection(int ref1, int ref2)
        {
            int index = GetConnectionIndex(ref1, ref2);

            if (index >= 0) { // Feedback connection is already added, increment counter
                IncConnection(index);
                return true;
            } else {
                return AddConnectionAux(ref1, ref2); // Add the feedback connection
            }
        }

        bool DecConnection(int ref1, int ref2)
        {
            int index = GetConnectionIndex(ref1, ref2);

            if (index >= 0) {
                jack_log("JackLoopFeedback::DecConnection ref1 = %ld ref2 = %ld index = %ld", ref1, ref2, index);
                return (DecConnection(index) == 0) ? RemoveConnectionAux(ref1, ref2) : true;
            } else {
                return false;
            }
        }

        /*!
        	\brief Test if a connection between 2 refnum is a feedback connection.
        */
        int GetConnectionIndex(int ref1, int ref2) const
        {
            for (int i = 0; i < SIZE; i++) {
                if (fTable[i][0] == ref1 && fTable[i][1] == ref2)
                    return i;
            }
            return -1;
        }

} POST_PACKED_STRUCTURE;

/*!
\brief For client timing measurements.
*/

PRE_PACKED_STRUCTURE
struct JackClientTiming
{
    jack_time_t fSignaledAt;
    jack_time_t fAwakeAt;
    jack_time_t fFinishedAt;
    jack_client_state_t fStatus;

    JackClientTiming()
    {
        Init();
    }
    ~JackClientTiming()
    {}

    void Init()
    {
        fSignaledAt = 0;
        fAwakeAt = 0;
        fFinishedAt = 0;
        fStatus = NotTriggered;
    }

} POST_PACKED_STRUCTURE;

/*!
\brief Connection manager.

<UL>
<LI>The <B>fConnection</B> array contains the list (array line) of connected ports for a given port.
<LI>The <B>fInputPort</B> array contains the list (array line) of input connected  ports for a given client.
<LI>The <B>fOutputPort</B> array contains the list (array line) of ouput connected  ports for a given client.
<LI>The <B>fConnectionRef</B> array contains the number of ports connected between two clients.
<LI>The <B>fInputCounter</B> array contains the number of input clients connected to a given for activation purpose.
</UL>
*/

PRE_PACKED_STRUCTURE
class SERVER_EXPORT JackConnectionManager
{

    private:

        JackFixedArray<CONNECTION_NUM_FOR_PORT> fConnection[PORT_NUM_MAX];  /*! Connection matrix: list of connected ports for a given port: needed to compute Mix buffer */
        JackFixedArray1<PORT_NUM_FOR_CLIENT> fInputPort[CLIENT_NUM];	/*! Table of input port per refnum : to find a refnum for a given port */
        JackFixedArray<PORT_NUM_FOR_CLIENT> fOutputPort[CLIENT_NUM];	/*! Table of output port per refnum : to find a refnum for a given port */
        JackFixedMatrix<CLIENT_NUM> fConnectionRef;						/*! Table of port connections by (refnum , refnum) */
        JackActivationCount fInputCounter[CLIENT_NUM];					/*! Activation counter per refnum */
        JackLoopFeedback<CONNECTION_NUM_FOR_PORT> fLoopFeedback;		/*! Loop feedback connections */

        bool IsLoopPathAux(int ref1, int ref2) const;

    public:

        JackConnectionManager();
        ~JackConnectionManager();

        // Connections management
        int Connect(jack_port_id_t port_src, jack_port_id_t port_dst);
        int Disconnect(jack_port_id_t port_src, jack_port_id_t port_dst);
        bool IsConnected(jack_port_id_t port_src, jack_port_id_t port_dst) const;

        /*!
          \brief Get the connection number of a given port.
        */
        jack_int_t Connections(jack_port_id_t port_index) const
        {
            return fConnection[port_index].GetItemCount();
        }

        jack_port_id_t GetPort(jack_port_id_t port_index, int connection) const
        {
            assert(connection < CONNECTION_NUM_FOR_PORT);
            return (jack_port_id_t)fConnection[port_index].GetItem(connection);
        }

        const jack_int_t* GetConnections(jack_port_id_t port_index) const;

        bool IncFeedbackConnection(jack_port_id_t port_src, jack_port_id_t port_dst);
        bool DecFeedbackConnection(jack_port_id_t port_src, jack_port_id_t port_dst);
        bool IsFeedbackConnection(jack_port_id_t port_src, jack_port_id_t port_dst) const;

        bool IsLoopPath(jack_port_id_t port_src, jack_port_id_t port_dst) const;
        void IncDirectConnection(jack_port_id_t port_src, jack_port_id_t port_dst);
        void DecDirectConnection(jack_port_id_t port_src, jack_port_id_t port_dst);

        // Ports management
        int AddInputPort(int refnum, jack_port_id_t port_index);
        int AddOutputPort(int refnum, jack_port_id_t port_index);

        int RemoveInputPort(int refnum, jack_port_id_t port_index);
        int RemoveOutputPort(int refnum, jack_port_id_t port_index);

        const jack_int_t* GetInputPorts(int refnum);
        const jack_int_t* GetOutputPorts(int refnum);

        // Client management
        void InitRefNum(int refnum);
        int GetInputRefNum(jack_port_id_t port_index) const;
        int GetOutputRefNum(jack_port_id_t port_index) const;

        // Connect/Disconnect 2 refnum "directly"
        bool IsDirectConnection(int ref1, int ref2) const;
        void DirectConnect(int ref1, int ref2);
        void DirectDisconnect(int ref1, int ref2);

        int GetActivation(int refnum) const
        {
            return fInputCounter[refnum].GetValue();
        }

        // Graph
        void ResetGraph(JackClientTiming* timing);
        int ResumeRefNum(JackClientControl* control, JackSynchro* table, JackClientTiming* timing);
        int SuspendRefNum(JackClientControl* control, JackSynchro* table, JackClientTiming* timing, long time_out_usec);
        void TopologicalSort(std::vector<jack_int_t>& sorted);

} POST_PACKED_STRUCTURE;

} // end of namespace

#endif

