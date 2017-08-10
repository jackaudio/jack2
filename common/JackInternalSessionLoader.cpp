/*
Copyright (C) 2017 Timo Wischer

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

#include <fstream>
#include "JackInternalSessionLoader.h"
#include "JackLockedEngine.h"


namespace Jack
{

JackInternalSessionLoader::JackInternalSessionLoader(JackServer* const server) :
    fServer(server)
{
}

int JackInternalSessionLoader::Load(const char* file)
{
    std::ifstream infile(file);

    if (!infile.is_open()) {
        jack_error("JACK internal session file %s does not exist or cannot be opened for reading.", file);
        return -1;
    }

    std::string line;
    int linenr = -1;
    while (std::getline(infile, line))
    {
        linenr++;

        std::istringstream iss(line);

        std::string command;
        if ( !(iss >> command) ) {
            /* ignoring empty line or line only filled with spaces */
            continue;
        }

        /* convert command to lower case to accept any case of the letters in the command */
        std::transform(command.begin(), command.end(), command.begin(), ::tolower);

        if ( (command.compare("c") == 0) || (command.compare("con") == 0) ) {
            ConnectPorts(iss, linenr);
        } else if ( (command.compare("l") == 0) || (command.compare("load") == 0) ) {
            LoadClient(iss, linenr);
#if 0
        /* NOTE: c++11 only */
        } else if (command.front() == '#') {
#else
        } else if (command[0] == '#') {
#endif
            /* ignoring commented lines.
             * The # can be followed by non spaces.
             * Therefore only compare the first letter of the command.
             */
        } else {
            jack_error("JACK internal session file %s line %u contains unkown command '%s'. Ignoring the line!", file, linenr, line.c_str());
        }
    }

    return 0;
}

void JackInternalSessionLoader::LoadClient(std::istringstream& iss, const int linenr)
{
    std::string client_name;
    if ( !(iss >> client_name) ) {
        jack_error("Cannot read client name from internal session file line %u '%s'. Ignoring the line!", linenr, iss.str().c_str());
        return;
    }

    std::string lib_name;
    if ( !(iss >> lib_name) ) {
        jack_error("Cannot read client library name from internal session file line %u '%s'. Ignoring the line!", linenr, iss.str().c_str());
        return;
    }

    /* get the rest of the line */
    std::string parameters;
    if ( std::getline(iss, parameters) ) {
        /* remove the leading spaces */
        const std::size_t start = parameters.find_first_not_of(" \t");
        if (start == std::string::npos) {
            /* Parameters containing only spaces.
             * Use empty parameter string.
             */
            parameters = "";
        } else {
            parameters = parameters.substr(start);
        }
    }


    /* jackctl_server_load_internal() can not be used
     * because it calls jack_internal_initialize()
     * instead of jack_initialize()
     */
    int status = 0;
    int refnum = 0;
    if (fServer->InternalClientLoad1(client_name.c_str(), lib_name.c_str(), parameters.c_str(), (JackLoadName|JackUseExactName|JackLoadInit), &refnum, -1, &status) < 0) {
        /* Due to the JackUseExactName option JackNameNotUnique will always handled as a failure.
         * See JackEngine::ClientCheck().
         */
        if (status & JackNameNotUnique) {
            jack_error("Internal client name `%s' not unique", client_name.c_str());
        }
        /* An error message for JackVersionError will already
         * be printed by JackInternalClient::Open().
         * Therefore no need to handle it here.
         */

        jack_error("Cannot load client %s from internal session file line %u. Ignoring the line!", client_name.c_str(), linenr);
        return;
    }

    /* status has not to be checked for JackFailure
     * because JackServer::InternalClientLoad1() will return a value < 0
     * and this is handled by the previouse if-clause.
     */

    jack_info("Internal client %s successfully loaded", client_name.c_str());
}

void JackInternalSessionLoader::ConnectPorts(std::istringstream& iss, const int linenr)
{
    std::string src_port;
    if ( !(iss >> src_port) ) {
        jack_error("Cannot read first port from internal session file line %u '%s'. Ignoring the line!",
                   linenr, iss.str().c_str());
        return;
    }

    std::string dst_port;
    if ( !(iss >> dst_port) ) {
        jack_error("Cannot read second port from internal session file line %u '%s'. Ignoring the line!",
                   linenr, iss.str().c_str());
        return;
    }

    /* use the client reference of the source port */
    const jack_port_id_t src_port_index = fServer->GetGraphManager()->GetPort(src_port.c_str());
    if (src_port_index >= NO_PORT) {
        jack_error("Source port %s does not exist! Ignoring internal session file line %u '%s'.",
                   src_port.c_str(), linenr, iss.str().c_str());
        return;
    }
    const int src_refnum = fServer->GetGraphManager()->GetOutputRefNum(src_port_index);

    if (fServer->GetEngine()->PortConnect(src_refnum, src_port.c_str(), dst_port.c_str()) < 0) {
        jack_error("Cannot connect ports of internal session file line %u '%s'.\n"
                   "Possibly the destination port does not exist. Ignoring the line!",
                   linenr, iss.str().c_str());
        return;
    }

    jack_info("Ports connected: %s -> %s", src_port.c_str(), dst_port.c_str());
}

}
