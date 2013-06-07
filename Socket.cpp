/**
 * @file Socket.cpp
 * @header Socket.h
 * @brief Definition of a socket that accepts connections from clients on a port.
 *
 * Adapted from CSS 432 Spring 2013 HW 1 by Derek Mclean
 *
 * @date 17-04-2013
 */

#include "Socket.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <iostream>
#include <stdlib.h>
#include <cstring>

/**
 * Creates an open connection to an open resources.
 *
 * @param address - the network location of the remote resource to connect to.
 */
Socket::Socket(const char *ip_address, const int port) :
    sockDescriptor(socket(AF_INET, SOCK_STREAM, 0))
{
    if (sockDescriptor < 0) {
        std::cerr << "Could not create socket!" << std::endl;
        exit(1);
    }

    sockaddr_in address;
    createAddress(ip_address, port, address);

    int result = connect(sockDescriptor, (sockaddr *) &address, sizeof(address));
    if (result < 0) {
        std::cerr << "Could not connect socket!" << std::endl;
        exit(1);
    }
}

/** DTR. Closes Socket. */
Socket::~Socket() {
    close();
}

/**
 * Set socket to be handled asynchronously by a previously setup
 * asynchronous io handler.
 */
void Socket::setAsync() const {
    fcntl(sockDescriptor, F_SETOWN, getpid());
    fcntl(sockDescriptor, F_SETFL, FASYNC);
}

/**
 * TODO
 */
void Socket::readInto(std::ostream &output) {
    // poll this socket for 1000msec (=1sec)
    char buf[1024];
    int nread;
    while (poll(1000)) {                  // the socket is ready to read
        nread = read<char>(buf, 1024);    // guaranteed to return from read
        if (nread == 0) {
            break;
        }                                                    // even if nread < BUFLEN
        output << std::string(buf, nread);
    }
}

/**
 * TODO
 */
void Socket::writeFrom(std::istream &input) {
    // TODO
}

/**
 * TODO
 */
bool Socket::poll(int timeout) {
    struct pollfd ufds;
    ufds.fd = sockDescriptor;         // a socket descriptor to exmaine for read
    ufds.events = POLLIN;             // check if this sd is ready to read
    ufds.revents = 0;                 // simply zero-initialized

    int numEvents = ::poll(&ufds, 1, 1000);
    return ufds.revents & POLLIN && numEvents > 0;
}

void Socket::shutdown() {
    ::shutdown(sockDescriptor, SHUT_WR);
}

/** Closes socket. Only call if necessary to close socket before destruction of Socket. */
void Socket::close() {
    ::close(sockDescriptor);
}

/**
 * @brief Creates a network address representation of remote resource.
 *
 * @param ip_address - internet address of remote resource
 * @param value - port on which to form a connection to the remote resource.
 *
 * @return address of resource
 */
void Socket::createAddress(const char *ip_address, const int port, sockaddr_in &address) {
    struct hostent* host = gethostbyname(ip_address);

    memset((char *)&address, '\0', sizeof(address));
    address.sin_family = AF_INET; // Address Family Internet
    bcopy(host->h_addr, (char *)&address.sin_addr, host->h_length);
    address.sin_port = htons(port);  // convert host byte-order
}
