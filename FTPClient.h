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
        bool executePassive(FTPClient::PassiveVisitor *visitor);
        void close(bool force=false);

    private:
        Socket *controlSocket;
        static const int DEFAULT_PORT;
};

class FTPClient::PassiveVisitor {
    public:
        virtual ~PassiveVisitor() { }
        virtual void handleControl(FTPClient &) =0;
        virtual void handleData(Socket &) =0;
};

#endif