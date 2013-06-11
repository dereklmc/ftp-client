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
        bool writeCmd(const std::string &cmd, std::ostream &output);
        Socket* openPassive(std::ostream &output);
        bool close(std::ostream &output, const bool force=false);
        bool pwd(std::ostream &out);
        const std::string getHostname(void) const;

        static const std::string END_LINE;
        static const int DEFAULT_PORT;

    private:
        Socket *controlSocket;
        std::string hostname;
};

#endif
