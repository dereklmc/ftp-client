/**
 * @file FTPClient.h
 * @brief Declaration for FTP Client.
 *
 * @author Derek McLean, Mitch Carlson
 *
 * @date 11-06-2013
 */

#ifndef __FTP_CLIENT__
#define __FTP_CLIENT__

#include <string>
#include <iostream>

#include "Socket.h"

/**
 * Encapsulates behavior of File Transfer Protocol over a TCP socket.
 *
 * Much of the behavior defined by this class either involves the underlying
 * control socket or common functions (such as opening a passive socket).
 * Specific commands not reused are handled on their own. This should change
 * in a future implementation.
 */
class FTPClient {
    public:

        /** CTR **/
        FTPClient();
        /** DTR - closes open connections */
        ~FTPClient();

        bool isOpen() const;
        bool open(std::string hostname, int port=DEFAULT_PORT);
        /** Read from the underlying socket into a given output stream. */
        void readInto(std::ostream &output);
        /** Write into the underlying socket from a given input stream. */
        void writeFrom(std::istream &input);
        /** Writes a given FTP command.
         *
         * Most of the commands are handled by subclassed of command. This
         * function provides a generic interface for them.
         */
        bool writeCmd(const std::string &cmd, std::ostream &output);
        /**
         * Create a new socket for passively sending or receiving data.
         */
        Socket* openPassive(std::ostream &output);
        /** Close current connection. #open should be called next, if at all */
        bool close(std::ostream &output, const bool force=false);
        /** Retrieves and writes the current working directory to an output source */
        bool pwd(std::ostream &out);
        /** Address of currently connected ftp server */
        const std::string getHostname(void) const;

        static const std::string END_LINE;
        static const int DEFAULT_PORT;

    private:
        Socket *controlSocket;
        /** Address of currently connected ftp server */
        std::string hostname;
};

#endif
