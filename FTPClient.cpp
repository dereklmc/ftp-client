#include "FTPClient.h"
#include <string.h> // strcpy, strcat
#include <stdio.h>  // printf

#include <sstream>

const int FTPClient::DEFAULT_PORT(21);

FTPClient::FTPClient(std::string hostname, int port) :
    controlSocket(hostname.c_str(), port), hostname(hostname)
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

void FTPClient::authorize(std::string input) {
     char buf[1024];
     strcpy(buf, input.c_str());
     strcat(buf, "\r\n");
     controlSocket.write<char>(buf, (int)input.length()+2);
}
const std::string FTPClient::getHostname(void) const {
    return hostname;
}
