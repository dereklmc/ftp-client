/**
 * @file Socket.h
 * @brief Declaration of a socket that connects to an open resource.
 *
 * Adapted from CSS 432 Spring 2013 HW 1 by Derek Mclean
 *
 * @date 17-04-2013
 */

#ifndef _SOCKET_
#define _SOCKET_

#include <netdb.h>
#include <unistd.h>         // read, write, close
 #include <sys/poll.h>      // polling

class Socket {
    public:
        /**
         * Creates an open connection to an open resources.
         *
         * @param address - the network location of the remote resource to connect to.
         */
        Socket(const char *ip_address, const int port);

        /** DTR. Closes Socket. */
        ~Socket();

        /**
         * Set socket to be handled asynchronously by a previously setup
         * asynchronous io handler.
         */
        void setAsync() const;

        /**
         * TODO
         */
         bool poll(int timeout = 0);

        /**
         * Read a buffer of data of some type from the remote destination.
         *
         * @param dataBuffer - buffer (array) of data to read into.
         * @param length - capacity of buffer.
         *
         * @return number of elements read.
         */
        template<typename T>
        int read(T *dataBuffer, const int length) const;

        /**
         * Write a buffer of data of some type to the remote destination.
         *
         * @param dataBuffer - buffer (array) of data to write.
         * @param length - capacity of buffer.
         */
        template<typename T>
        void write(T *dataBuffer, const int length) const;

        void shutdown();


    private:
        /** Reference to actual socket */
        const int sockDescriptor;
        struct pollfd ufds;

        /** Closes socket. Only call if necessary to close socket before destruction of Socket. */
        void close();

        void createAddress(const char *ip_address, const int port, sockaddr_in &address);
};

/**
 * Read a buffer of data of some type from the remote destination.
 *
 * @param dataBuffer - buffer (array) of data to read into.
 * @param length - capacity of buffer.
 *
 * @return number of elements read.
 */
template<typename T>
int Socket::read(T *dataBuffer, const int length) const {
    return ::read(sockDescriptor, dataBuffer, length);
}

/**
 * Write a buffer of data of some type to the remote destination.
 *
 * @param dataBuffer - buffer (array) of data to write.
 * @param length - capacity of buffer.
 */
template<typename T>
void Socket::write(T *dataBuffer, const int length) const {
    ::write(sockDescriptor, dataBuffer, length);
}

#endif
