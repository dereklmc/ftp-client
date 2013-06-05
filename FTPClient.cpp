#include "FTPClient.h"

#include <sstream>
#include <boost/regex.hpp>

const int FTPClient::DEFAULT_PORT(21);

FTPClient::FTPClient(){
    controlSocket = NULL;
}

FTPClient::FTPClient(std::string hostname, int port) {
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

void FTPClient::readInto(std::ostream &output) {
    controlSocket->readInto(output);
}

void FTPClient::writeFrom(std::istream &input) {
    controlSocket->writeFrom(input);
}

bool FTPClient::executePassive(PassiveVisitor *visitor) {
    const char *pasvCmd = "PASV\r\n";
    controlSocket->write<char>(pasvCmd, 6);

    stringstream responseStream;
    controlSocket->readInto(responseStream);

    boost::regex responsePattern("\\d{3}[^\r\n]+ \\((\\d+),(\\d+),(\\d+),(\\d+),(\\d+),(\\d+)\\).\r\n");
    boost::smatch passiveMatch;
    bool found = boost::regex_search(responseStream.str(), passiveMatch, responsePattern);

    if (!found) {
        return false;
    }

    stringstream hostStream;
    hostStream << passiveMatch[1] << ".";
    hostStream << passiveMatch[2] << ".";
    hostStream << passiveMatch[3] << ".";
    hostStream << passiveMatch[4];

    int port = atoi(passiveMatch[5]) * 256 + atoi(passiveMatch[6]);

    Socket dataSocket(hostStream.str(), port);
    // fork!
    // in parent process:
    visitor.handleControl(*this);
    // in child process:
    visitor.handleData(dataSocket);
    // in parent process, join on fork
    // Close socket

    return true;
}

bool FTPClient::close(bool force) {
    if (!isOpen()) {
        return false;
    }
    const char *closeCmdString = "QUIT\r\n";
    controlSocket->write<char>(closeCmdString, 6);

    delete controlSocket;
    controlSocket = NULL;
    return true;
}