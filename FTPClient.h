#ifndef __FTP_CLIENT__
#define __FTP_CLIENT__

#include <string>

#include "Socket.h"

class FTPClient {
    public:
        FTPClient();
        FTPClient(std::string hostname, int port=DEFAULT_PORT);
        ~FTPClient();

        bool isOpen() const;
        bool open(std::string hostname, int port=DEFAULT_PORT);
        std::string read();
        void close();

    private:
        Socket *controlSocket;
        static const int DEFAULT_PORT;
};

#endif