#include "FTPClient.h"

#include <sstream>

const int FTPClient::DEFAULT_PORT(21);

FTPClient::FTPClient()
{
    controlSocket = NULL;
}

FTPClient::FTPClient(std::string hostname, int port)
{
    controlSocket = new Socket(hostname.c_str(), port);
}

FTPClient::~FTPClient()
{
    if (isOpen()) {
        close()
    }
}

bool FTPClient::isOpen() const {
    return controlSocket != NULL;
}

bool FTPClient::open(std::string hostname, int port) {
    if (isOpen()) {
        return false;
    }
    controlSocket = new Socket(hostname.c_str(), port);
    return true;
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

bool FTPClient::close() {
    if (!isOpen()) {
        return false;
    }
    delete controlSocket;
    controlSocket = NULL;
    return true;
}