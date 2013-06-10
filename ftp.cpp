// Library
#include <iostream>
#include <map>
#include <memory>  // For auto_ptr, which is deprecated in C++11 (gcc v4.7+)
#include <stdlib.h>
#include <sstream> // stringstream
#include <fstream>

// Boost
#include <boost/regex.hpp>

// Local
#include "ArgParse.h"
#include "CommandParser.h"
#include "Command.h"
#include "Context.h"
#include "FTPClient.h"

// Open (read/write) file in client/server + system rwx flags
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

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
            context.ftp.writeCmd("USER " + input + FTPClient::END_LINE);
            context.ftp.readInto(*context.output);
            std::cout << "Password: ";
            #ifdef DEBUG
            input = "UWB0th3ll";
            #endif
            #ifndef DEBUG
            *context.input >> input;
            #endif
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

            pid_t pid;
            if((pid = fork()) == -1) {
                perror("fork error");
                exit(EXIT_FAILURE);
            }
            /* Block on read() */
            else if (pid == 0) {                    // child process
                dataSocket->readInto(*context.output);
                exit(EXIT_SUCCESS);
            }
            /* Send directory list to data port */
            else {                                  // parent process
                context.ftp.writeCmd("LIST" + FTPClient::END_LINE);
                context.ftp.readInto(*context.output);
            }
            delete dataSocket;
            dataSocket = NULL;
        } else {
            *context.output << "Could not establish data connection." << std::endl;
        }
    }
};

class GetCmd : public Command {
public:
    void execute(Context &context) {
        std::string fileName;
        *context.input >> fileName;
        mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

        if(open( fileName.c_str(), O_WRONLY | O_CREAT, mode )) {
            Socket *dataSocket = context.ftp.openPassive(*context.output);  // send PASV command

            if (dataSocket != NULL) {

                pid_t pid;
                if((pid = fork()) == -1) {
                    perror("fork error");
                    exit(EXIT_FAILURE);
                }
                /* Block on read() */
                else if (pid == 0) {                        // child process
                    std::fstream file;
                    file.open(fileName.c_str(), std::fstream::in |
                        std::fstream::out);
                    dataSocket->readInto(file);
                    file.close();
                    exit(EXIT_SUCCESS);
                }
                /*  */
                else {                                      // parent process
                    context.ftp.writeCmd("TYPE I" + FTPClient::END_LINE);
                    context.ftp.writeCmd("RETR " + fileName +
                        FTPClient::END_LINE);
                    context.ftp.readInto(*context.output);  // get reply
                }
                delete dataSocket;
                dataSocket = NULL;

            } else {
                *context.output << "Could not establish data connection." << std::endl;
            }
        } else {
            *context.output << "File error" << std::endl;
        }
    }
};

class PutCmd : public Command {
public:
    void execute(Context &context) {
        std::string localFile;
        std::string remoteFile;
        #ifdef DEBUG
        localFile = "mytest.txt";
        remoteFile =  "testing";
        #endif
        #ifndef DEBUG
        *context.output << "(local-file) ";
        *context.input >> localFile;
        *context.output << "(remote-file) ";
        *context.input >> remoteFile;
        #endif
        mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

        int dirfd = open( remoteFile.c_str(), O_DIRECTORY | O_RDONLY, mode );
        fchdir( dirfd );

        Socket *dataSocket = context.ftp.openPassive(*context.output);  // send PASV command

        if (dataSocket != NULL) {

            pid_t pid;
            if((pid = fork()) == -1) {
                perror("fork error");
                exit(EXIT_FAILURE);
            }
            /* Block on read() */
            else if (pid == 0) {                        // child process
                std::fstream file;
                file.open(localFile.c_str(), std::fstream::in |
                    std::fstream::out);
                dataSocket->writeFrom(file);
                file.close();
                exit(EXIT_SUCCESS);
            }
            /*  */
            else {                                      // parent process
                context.ftp.writeCmd("TYPE I" + FTPClient::END_LINE);
                context.ftp.writeCmd("STOR " + remoteFile +
                    FTPClient::END_LINE);
                int status = 0;
                waitpid(pid,&status,0);
                context.ftp.readInto(*context.output);
            }
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
