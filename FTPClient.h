#ifndef __FTP_CLIENT__
#define __FTP_CLIENT__

#include <string>
#include <iostream>

#include "Socket.h"

class FTPClient {
    public:
        class PassiveVisitor;

        FTPClient();
        FTPClient(std::string hostname, int port=DEFAULT_PORT);
        ~FTPClient();

        bool isOpen() const;
        bool open(std::string hostname, int port=DEFAULT_PORT);
        void readInto(std::ostream &output);
        void writeFrom(std::istream &input);
        void writeCmd(const std::string &cmd);
        Socket* openPassive();
        bool close(std::ostream *output = NULL, const bool force=false);

        const std::string getHostname(void) const;
        void authorize(std::string);
        void pwd(std::ostream &out);

    private:
        Socket *controlSocket;
        static const int DEFAULT_PORT;
        static const std::string END_LINE;
        std::string hostname;
};

#endif
