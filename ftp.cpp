// Library
#include <iostream>
#include <map>
#include <memory>  // For auto_ptr, which is deprecated in C++11 (gcc v4.7+)
#include <stdlib.h>
#include <sstream>
#include <pwd.h>

// Local
#include "Command.h"
#include "Context.h"
#include "FTPClient.h"
#include "debug.h"

//#define DEBUG

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
            struct passwd *pass;
            pass = getpwuid(getuid());
            std::string netid(pass->pw_name);
            std::string input;

            *context.output << "Name ("
                            << context.ftp.getHostname()
                            << ":" << netid << "): ";
            *context.input >> input;
            DEBUG(std::cout << "D" << std::endl);
            context.ftp.writeCmd("USER " + input + FTPClient::END_LINE);
            context.ftp.readInto(*context.output);
            std::cout << "Password: ";
            *context.input >> input;
            context.ftp.writeCmd("PASS " + input + FTPClient::END_LINE);
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
            if (context.ftp.isOpen()) {
                context.ftp.close(context.output);
            } else {
                *context.output << "No connection to close." << std::endl;
            }
        }
};

class QuitCmd : public CloseCmd {
    public:
        void execute(Context &context) {
            CloseCmd::execute(context);
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

    std::map<std::string,Command*> commands;
    commands["open"] = open.get();
    commands["quit"] = quit.get();
    commands["cd"] = cd.get();
    commands["close"] = close.get();
    commands["ls"] = ls.get();
    commands["get"] = get.get();
    commands["put"] = put.get();
    commands["pwd"] = pwd.get();
    commands["mkdir"] = mkdir.get();
    Context context(std::cin, std::cout);
    std::string shellName = "ftp";
    while (1) {
        std::string commandStr;
        std::cout << shellName << ":" << context.workingDirectory << "> " << std::flush;
        std::cin >> commandStr;


        std::map<std::string,Command*>::iterator itCmd = commands.find(commandStr);
        if (itCmd == commands.end()) {
            std::cerr << "Command Does Not Exist!" << std::endl;
        } else {
            itCmd->second->execute(context);
        }
    }

}

// open ftp.tripod.com 21
