// Library
#include <iostream>
#include <map>
#include <memory>  // For auto_ptr, which is deprecated in C++11 (gcc v4.7+)
#include <stdlib.h>
#include <sstream>

// Boost
#include <boost/regex.hpp>

// Local
#include "ArgParse.h"
#include "CommandParser.h"
#include "Command.h"
#include "Context.h"
#include "FTPClient.h"

// Declarations

// Definitions

class OpenCmd : public Command {
    public:
        void execute(Context &context) {
            std::string hostname;
            int port;

            *context.input >> hostname >> port;
            *context.output << "Open connection to \"" << hostname << ":" << port << "\"" << std::endl;
            context.ftp.open(hostname, port);
            context.ftp.readInto(*context.output);

            authorize(context);

            std::stringstream currDirStream;
            context.ftp.pwd(currDirStream);
            int pwd_code;
            currDirStream >> pwd_code;
            currDirStream >> context.workingDirectory;
        }

        void authorize(Context &context) {
            std::string netid(getlogin());
            std::string input;

            *context.output << "Name ("
                            << context.ftp.getHostname()
                            << ":" << netid << "): ";
            *context.input >> input;
            context.ftp.authorize("USER " + input);
            context.ftp.readInto(*context.output);
            std::cout << "Password: ";
            *context.input >> input;
            context.ftp.authorize("PASS " + input);
            context.ftp.readInto(*context.output);
        }
};

class PWDCmd : public Command {
    public:
        void execute(Context &context) {
            context.ftp.pwd(*context.output);
        }
};

class CloseCmd : public Command {
    public:
        void execute(Context &context) {
            context.ftp.close(context.output);
        }
};

class QuitCmd : public Command {
    public:
        void execute(Context &context) {
            if (context.ftp.isOpen()) {
                context.ftp.close(context.output);
            }
            *context.output << "GOODBYE!" << std::endl;
            exit(0);
        }
};

class MkdirCmd : public Command {
    public:
        void execute(Context &context) {
            if (context.ftp.isOpen()) {
                std::string dir;
                *context.input >> dir;
                context.ftp.writeCmd("MKD " + dir + FTPClient::END_LINE);
            }
        }
};

class CdCmd : public Command {
public:
    void execute(Context &context) {
        std::string directory;
        *context.input >> directory;
        context.ftp.writeCmd("CWD " + directory + FTPClient::END_LINE);

        context.ftp.readInto(*context.output);

        std::stringstream currDirStream;
        context.ftp.pwd(currDirStream);
        int pwd_code;
        currDirStream >> pwd_code;
        currDirStream >> context.workingDirectory;
    }
};

class LsCmd : public Command {
public:
    void execute(Context &context) {
        Socket *dataSocket = context.ftp.openPassive();                     // FTP server PASV command
        if (dataSocket != NULL) {
            context.ftp.readInto(*context.output);         // get reply from socket
            context.ftp.list("TODO: remove str");       // FTP server LIST command
            dataSocket->readInto(*context.output);

            delete dataSocket;
            dataSocket = NULL;
        } else {
            *context.output << "Could not establish data connection." << std::endl;
        }
    }
};

int main(int argc, char *argv[]) {

    ArgParse argparser;
    argparser.addArgument("ftpserver", false);
    if (!argparser.parse(argc, argv)) {
        std::cout << "usage: " << argparser.usage(argv[0]) << std::endl;
    }

    std::auto_ptr<Command> open(new OpenCmd());
    std::auto_ptr<Command> quit(new QuitCmd());
    std::auto_ptr<Command> cd(new CdCmd());
    std::auto_ptr<Command> close(new CloseCmd());
    std::auto_ptr<Command> ls(new LsCmd());
    std::auto_ptr<Command> pwd(new PWDCmd());
    std::auto_ptr<Command> mkdir(new MkdirCmd());
    CommandParser cmdParser("ftp");
    cmdParser.addCommand("open", open.get());
    cmdParser.addCommand("quit", quit.get());
    cmdParser.addCommand("cd", cd.get());
    cmdParser.addCommand("close", close.get());
    cmdParser.addCommand("ls", ls.get());
    cmdParser.addCommand("pwd", pwd.get());
    cmdParser.addCommand("mkdir", mkdir.get());

    Context context(std::cin, std::cout);
    while (1) {
        cmdParser.parse(context);
    }

}

// open ftp.tripod.com 21
