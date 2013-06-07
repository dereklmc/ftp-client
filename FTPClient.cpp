#include "FTPClient.h"
#include <string.h> // strcpy, strcat
#include <stdio.h>  // printf

#include <sstream>
#include <boost/regex.hpp>

const int FTPClient::DEFAULT_PORT(21);

FTPClient::FTPClient() {
    controlSocket = NULL;
}

FTPClient::FTPClient(std::string hostname, int port) :
    hostname(hostname)
{
    controlSocket = new Socket(hostname.c_str(), port);
}

FTPClient::~FTPClient()
{
    if (isOpen()) {
        close();
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

void FTPClient::readInto(std::ostream &output) {
    controlSocket->readInto(output);
}

void FTPClient::writeFrom(std::istream &input) {
    controlSocket->writeFrom(input);
}

Socket* FTPClient::openPassive() {
    const char *pasvCmd = "PASV\r\n";
    controlSocket->write<const char>(pasvCmd, 6);

    std::stringstream responseStream;
    controlSocket->readInto(responseStream);

    boost::regex responsePattern("\\d{3}[^\r\n]+ \\((\\d+),(\\d+),(\\d+),(\\d+),(\\d+),(\\d+)\\).\r\n");
    boost::smatch passiveMatch;
    bool found = boost::regex_search(responseStream.str(), passiveMatch, responsePattern);

    if (!found) {
        return NULL;
    }

    std::string host;
    host.append(passiveMatch[1].first, passiveMatch[1].second);
    host.append(".");
    host.append(passiveMatch[2].first, passiveMatch[2].second);
    host.append(".");
    host.append(passiveMatch[3].first, passiveMatch[3].second);
    host.append(".");
    host.append(passiveMatch[4].first, passiveMatch[1].second);

    std::string p1(passiveMatch[5].first, passiveMatch[5].second);
    std::string p2(passiveMatch[6].first, passiveMatch[6].second);
    int port = atoi(p1.c_str()) * 256 + atoi(p2.c_str());

    return new Socket(host.c_str(), port);
}

bool FTPClient::close(bool force) {
    if (!isOpen()) {
        return false;
    }
    const char *closeCmdString = "QUIT\r\n";
    controlSocket->write<const char>(closeCmdString, 6);

    delete controlSocket;
    controlSocket = NULL;
    return true;
}

void FTPClient::authorize(std::string input) {
     char buf[1024];
     strcpy(buf, input.c_str());
     strcat(buf, "\r\n");
     controlSocket->write<char>(buf, (int)input.length()+2);
}
const std::string FTPClient::getHostname(void) const {
    return hostname;
}
