#include "FTPClient.h"
#include <string.h> // strcpy, strcat
#include <stdio.h>  // printf
#include "Socket.h"
#include <sstream>  // istringstream, ostringstream
#include <sys/types.h>

#include "debug.h"

const int FTPClient::DEFAULT_PORT(21);
const std::string FTPClient::END_LINE("\r\n");

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
    this->hostname = hostname;
    controlSocket = new Socket(hostname.c_str(), port);
    return true;
}

void FTPClient::readInto(std::ostream &output) {
    controlSocket->readInto(output);
}

void FTPClient::writeFrom(std::istream &input) {
    controlSocket->writeFrom(input);
}

Socket* FTPClient::openPassive(std::ostream &output) {
    const char *pasvCmd = "PASV\r\n";
    controlSocket->write<const char>(pasvCmd, 6);

    std::stringstream responseStream;
    controlSocket->readInto(responseStream);
    output << responseStream.str();

    int code = 0;
    int host1, host2, host3, host4, port1, port2;

    responseStream >> code;
    if (code != 227) {
        return NULL;
    }
    responseStream.ignore(200,'(');
    responseStream >> host1; responseStream.ignore();
    responseStream >> host2; responseStream.ignore();
    responseStream >> host3; responseStream.ignore();
    responseStream >> host4; responseStream.ignore();
    std::stringstream hostStream;
    hostStream << host1 << "." << host2 << "." << host3 << "." << host4;
    std::string host = hostStream.str();

    responseStream >> port1; responseStream.ignore();
    responseStream >> port2;
    int port = port1 * 256 + port2;

    return new Socket(host.c_str(), port);
}

bool FTPClient::close(std::ostream *output, const bool force) {
    if (!isOpen()) {
        return false;
    }
    const char *closeCmdString = "QUIT\r\n";
    controlSocket->write<const char>(closeCmdString, 6);
    if (output != NULL) {
        readInto(*output);
    }

    delete controlSocket;
    controlSocket = NULL;
    return true;
}

void FTPClient::store(const std::string &input) {

}

void FTPClient::retrieve(const std::string &input) {
    std::string retrCmdStr = "retr " + input + "\r\n";
    controlSocket->write<const char>(retrCmdStr.c_str(),retrCmdStr.size());
}

const std::string FTPClient::getHostname(void) const {
    return hostname;
}

void FTPClient::writeCmd(const std::string &cmd) {
    controlSocket->write<const char>(cmd.c_str(), cmd.size());
}

void FTPClient::pwd(std::ostream &out) {
    writeCmd("PWD" + END_LINE);
    readInto(out);
}
