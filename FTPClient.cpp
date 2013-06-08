#include "FTPClient.h"
#include <string.h> // strcpy, strcat
#include <stdio.h>  // printf
#include <boost/regex.hpp>
#include "Socket.h"
#include <sstream>  // istringstream, ostringstream

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

Socket* FTPClient::openPassive() {
    const char *pasvCmd = "PASV\r\n";
    controlSocket->write<const char>(pasvCmd, 6);

    std::stringstream responseStream;
    controlSocket->readInto(responseStream);

    boost::regex responsePattern("\\d{3}[^\r\n]+ \\((\\d+),(\\d+),(\\d+),(\\d+),(\\d+),(\\d+)\\).\r\n");
    boost::smatch passiveMatch;
    bool found = boost::regex_search(responseStream.str(), passiveMatch, responsePattern);

    if (!found) {
        return NULL;
    }

    std::string host;
    host.append(passiveMatch[1].first, passiveMatch[1].second);
    host.append(".");
    host.append(passiveMatch[2].first, passiveMatch[2].second);
    host.append(".");
    host.append(passiveMatch[3].first, passiveMatch[3].second);
    host.append(".");
    host.append(passiveMatch[4].first, passiveMatch[1].second);

    std::string p1(passiveMatch[5].first, passiveMatch[5].second);
    std::string p2(passiveMatch[6].first, passiveMatch[6].second);
    int port = atoi(p1.c_str()) * 256 + atoi(p2.c_str());

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

void FTPClient::authorize(std::string input) const {
    char buf[1024];
    strcpy(buf, input.c_str());
    strcat(buf, "\r\n");
    controlSocket->write<char>(buf, (int)input.length()+2);
}

void FTPClient::pasv(void) const {
    char pasv[7] = "pasv\r\n";
    controlSocket->write<char>(pasv,6);
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
    controlSocket->write<char>(list,6);

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
