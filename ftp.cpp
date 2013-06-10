// Library
#include <iostream>
#include <map>
#include <memory>  // For auto_ptr, which is deprecated in C++11 (gcc v4.7+)
#include <stdlib.h>
#include <sstream>


// Local
#include "CommandParser.h"
#include "Command.h"
#include "Context.h"
#include "FTPClient.h"

#define DEBUG

// Declarations

// Definitions

class OpenCmd : public Command {
    public:
        void execute(Context &context) {
            std::string hostname;
            int port;
            #ifdef DEBUG
            hostname = "ftp.tripod.com";
            port = 21;
            #endif
            #ifndef DEBUG
            *context.input >> hostname >> port;
            #endif
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
            #ifdef DEBUG
            input = "css432";
            #endif
            #ifndef DEBUG
            *context.input >> input;
            #endif
            context.ftp.authorize("USER " + input);
            context.ftp.readInto(*context.output);
            std::cout << "Password: ";
            #ifdef DEBUG
            input = "UWB0th3ll";
            #endif
            #ifndef DEBUG
            *context.input >> input;
            #endif
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
        Socket *dataSocket = context.ftp.openPassive(*context.output);  // FTP server PASV command
        if (dataSocket != NULL) {

            pid_t pid = fork();

            /* Block on read() */
            if (pid == 0) {         // child proc
                dataSocket->readInto(*context.output);
                delete dataSocket;
                dataSocket = NULL;
                exit(0);
            }
            /* Send directory list to data port */
            else if (pid > 0) {     // parent
                context.ftp.writeCmd("LIST" + FTPClient::END_LINE);
                context.ftp.readInto(*context.output);
                delete dataSocket;
                dataSocket = NULL;
            }
            else if (pid < 0) {     // failed
                std::ostringstream error("Process failed to fork");
                dataSocket->readInto(error);
                delete dataSocket;
                dataSocket = NULL;
                exit(1);
            }
        } else {
            *context.output << "Could not establish data connection." << std::endl;
        }
    }
};

class GetCmd : public Command {
public:
    void execute(Context &context) {
        std::string file;
        *context.input >> file;

        Socket *dataSocket = context.ftp.openPassive(*context.output);  // send PASV command

        if (dataSocket != NULL) {

            pid_t pid = fork();

            /* Block on read() */
            if (pid == 0) {                                       // child proc
                dataSocket->readInto(*context.output);
                delete dataSocket;
                dataSocket = NULL;
                exit(0);
            }
            /*  */
            else if (pid > 0) {                                   // parent
                context.ftp.writeCmd("TYPE I" + FTPClient::END_LINE);
                context.ftp.writeCmd("RETR " + file + FTPClient::END_LINE);
                context.ftp.readInto(*context.output);  // get reply
                delete dataSocket;
                dataSocket = NULL;
            }
            else if (pid < 0) {                                   // failed
                std::ostringstream error("Process failed to fork");
                dataSocket->readInto(error);
                delete dataSocket;
                dataSocket = NULL;
                exit(1);
            }
        } else {
            *context.output << "Could not establish data connection." << std::endl;
        }
    }
};

class PutCmd : public Command {
public:
    void execute(Context &context) {
    }
};

int main(int argc, char *argv[]) {

    std::string hostname;
    if (argc > 1) {
        std::string arg1 = argv[1];
        if (arg1 == "help") {
            std::cout << "usage: " << argv[0] << "[ftpserver]" << std::endl;
        } else {
            hostname = arg1;
        }
    }

    std::auto_ptr<Command> open(new OpenCmd());
    std::auto_ptr<Command> quit(new QuitCmd());
    std::auto_ptr<Command> cd(new CdCmd());
    std::auto_ptr<Command> close(new CloseCmd());
    std::auto_ptr<Command> ls(new LsCmd());
    std::auto_ptr<Command> get(new GetCmd());
    std::auto_ptr<Command> put(new PutCmd());
    std::auto_ptr<Command> pwd(new PWDCmd());
    std::auto_ptr<Command> mkdir(new MkdirCmd());
    CommandParser cmdParser("ftp");
    cmdParser.addCommand("open", open.get());
    cmdParser.addCommand("quit", quit.get());
    cmdParser.addCommand("cd", cd.get());
    cmdParser.addCommand("close", close.get());
    cmdParser.addCommand("ls", ls.get());
    cmdParser.addCommand("get", get.get());
    cmdParser.addCommand("put", put.get());
    cmdParser.addCommand("pwd", pwd.get());
    cmdParser.addCommand("mkdir", mkdir.get());

    Context context(std::cin, std::cout);
    while (1) {
        cmdParser.parse(context);
    }

}

// open ftp.tripod.com 21
