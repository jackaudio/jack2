#ifndef JACKLOADER_H
#define JACKLOADER_H

#include <string>
#include <sstream>
#include "JackServer.h"


namespace Jack
{

class JackLoader
{
public:
    JackLoader(JackServer* const server);
    int Load(const std::string file);

private:
    void LoadClient(std::istringstream& iss, const int linenr);
    void ConnectPorts(std::istringstream& iss, const int linenr);

    JackServer* const fServer;
};

} // end of namespace

#endif // JACKLOADER_H
