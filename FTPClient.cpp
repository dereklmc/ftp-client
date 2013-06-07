#include "FTPClient.h"
#include <string.h> // strcpy, strcat
#include <stdio.h>  // printf
#include "Socket.h"
#include <sstream>  // istringstream, ostringstream

const int FTPClient::DEFAULT_PORT(21);

FTPClient::FTPClient(std::string hostname, int port) :
    controlSocket(hostname.c_str(), port), hostname(hostname)
{
    // ctr
}

std::string FTPClient::read() {

    std::stringstream rss;

    // poll this socket for 1000msec (=1sec)
    while (controlSocket.poll(1000)) {     // the socket is ready to read
        char buf[1024];
        int nread = controlSocket.read<char>(buf, 1024); // guaranteed to return from read
                                           // even if nread < BUFLEN
        rss << std::string(buf, nread);
    }
    return rss.str();
}

void FTPClient::authorize(std::string input) const {
    char buf[1024];
    strcpy(buf, input.c_str());
    strcat(buf, "\r\n");
    controlSocket.write<char>(buf, (int)input.length()+2);
}

void FTPClient::pasv(void) const {
    char pasv[7] = "pasv\r\n";
    controlSocket.write<char>(pasv,6);
}

void FTPClient::list(const std::string ftpReply) {
    using namespace std;

    string host;
    int port;

    /* Parse FTP reply and connect to data port */
    parse(ftpReply, host, port);
    dataSocket = new Socket(host.c_str(), port);

    /* Send directory list to data port */
    char list[7] = "list\r\n";
    controlSocket.write<char>(list,6);

}

const std::string FTPClient::getHostname(void) const {
    return hostname;
}

void FTPClient::parse(std::string ftpReply, std::string &host, int &port) const {
    using namespace std;
    const int MULT = 256;       // constant multiplier to find port

    int first, last, ports[2];  // index, index, numbers to find port
    ostringstream convert_to;   // convert to string
    istringstream parser;       // convert to int

    /* Extract substring */
    first = ftpReply.find_first_of('(')+1;
    last  = ftpReply.find_last_of(')');
    ftpReply  = ftpReply.substr(first,last-first);

    /* Replace all commas */
    for (int i = 0; i < 3; i++)
        ftpReply.replace(ftpReply.find_first_of(','),1,".");
    for (int i = 0; i < 2; i++)
        ftpReply.replace(ftpReply.find_first_of(','),1," ");
    parser.str(ftpReply);
    parser >> host;

    /* Find port number */
    for (int i = 0; i < 2; i++) parser >> ports[i];
    port = ports[0]*MULT+ports[1];
}
