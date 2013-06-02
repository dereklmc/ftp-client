#include "FTPClient.h"

#include <sstream>

const int FTPClient::DEFAULT_PORT(21);

FTPClient::FTPClient(std::string hostname, int port) :
    controlSocket(hostname.c_str(), port)
{
    // ctr
}

std::string FTPClient::read() {

    std::stringstream rss;

    // poll this socket for 1000msec (=1sec)
    while (controlSocket.poll(1000)) {                  // the socket is ready to read
        char buf[1024];
        int nread = controlSocket.read<char>(buf, 1024); // guaranteed to return from read
                                           // even if nread < BUFLEN
        rss << std::string(buf, nread);
    }
    return rss.str();
}