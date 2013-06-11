/**
 * @file FTPClient.cpp
 * @brief Definitions for FTP Client.
 *
 * @author Derek McLean, Mitch Carlson
 *
 * @date 11-06-2013
 */

#include "FTPClient.h"
#include <string.h> // strcpy, strcat
#include <stdio.h>  // printf
#include "Socket.h"
#include <sstream>  // istringstream, ostringstream
#include <sys/types.h>

#include "debug.h"

// FTP return code if the serve is closing the connection.
#define CLOSE_CONN 421

const int FTPClient::DEFAULT_PORT(21);
const std::string FTPClient::END_LINE("\r\n");

/** ctr **/
FTPClient::FTPClient() {
    controlSocket = NULL;
}

/** DTR - closes open connections */
FTPClient::~FTPClient()
{
    if (isOpen()) {
        close(std::cerr);
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

/**
 * Read from the underlying socket into a given output stream.
 *
 * @param output - the stream to write read data to.
 *
 * @see Socket#readInto
 */
void FTPClient::readInto(std::ostream &output) {
    controlSocket->readInto(output);
}

/**
 * Write to the underlying socket from a given input stream.
 *
 * @param input - the stream to read from when writing to the socket.
 *
 * @see Socket#writeFrom
 */
void FTPClient::writeFrom(std::istream &input) {
    controlSocket->writeFrom(input);
}

/**
 * Establishes a new passive connection with the current ftp server.
 *
 * Request an address and port to establish a connection on, then establishes
 * said connection.
 *
 * @param output - where to write results to.
 *
 * @return NULL if an error occured and/or the connection could not be established
 *         otherwise, returns a new tcp socket. Close socket when done.
 */
Socket* FTPClient::openPassive(std::ostream &output) {
    std::stringstream responseStream;
    if (!writeCmd("PASV" + END_LINE, responseStream)) {
        return NULL;
    }

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
    DEBUG(std::cout << "Open data connection to \"" << host << ":" << port << "\"" << std::endl;)
    return new Socket(host.c_str(), port);
}

bool FTPClient::close(std::ostream &output, const bool force) {
    if (!writeCmd("QUIT" + END_LINE, output)) {
        return false;
    }

    delete controlSocket;
    controlSocket = NULL;
    return true;
}

const std::string FTPClient::getHostname(void) const {
    return hostname;
}

/**
 * Generic handler for sending arbitrary ftp commands to the current ftp server.
 *
 * Handles errors: currently handles code 421. This should be expanded in future
 * implementations.
 */
bool FTPClient::writeCmd(const std::string &cmd, std::ostream &output) {
    if (!isOpen()) {
        return false;
    }
    controlSocket->write<const char>(cmd.c_str(), cmd.size());
    std::stringstream result;
    int returnCode;

    readInto(result);
    output << result.str();

    result >> returnCode;
    if (returnCode == CLOSE_CONN) {
        delete controlSocket;
        controlSocket = NULL;
        output << "Connection closed by server!" << std::endl;
        return false;
    }
    return true;
}

/** Print working directory to some output source, like a console. */
bool FTPClient::pwd(std::ostream &out) {
    return writeCmd("PWD" + END_LINE, out);
}
