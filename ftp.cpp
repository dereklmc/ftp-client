// Library
#include <iostream>
#include <map>
#include <memory>  // For auto_ptr, which is deprecated in C++11 (gcc v4.7+)
#include <stdlib.h>
#include <pwd.h>
#include <sstream> // stringstream
#include <fstream>

// Local
#include "Command.h"
#include "Context.h"
#include "FTPClient.h"
#include "debug.h"

// Open (read/write) file in client/server + system rwx flags
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

// Declarations

// Definitions

class OpenCmd : public Command {
    public:
        Command::Status execute(Context &context) {
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
            return OK;
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
        Command::Status execute(Context &context) {
            context.ftp.pwd(*context.output);
            return OK;
        }
};

class CloseCmd : public Command {
    public:
        Command::Status execute(Context &context) {
            if (context.ftp.isOpen()) {
                context.ftp.close(context.output);
                return OK;
            } else {
                *context.output << "No connection to close." << std::endl;
                return ERROR;
            }
        }
};

class QuitCmd : public CloseCmd {
    public:
        Command::Status execute(Context &context) {
            CloseCmd::execute(context);
            *context.output << "GOODBYE!" << std::endl;
            //exit(0);
            return EXIT;
        }
};

class MkdirCmd : public Command {
    public:
        Command::Status execute(Context &context) {
            if (!context.ftp.isOpen()) {
                return ERROR;
            }
            std::string dir;
            *context.input >> dir;
            context.ftp.writeCmd("MKD " + dir + FTPClient::END_LINE);
            return OK;
        }
};

class CdCmd : public Command {
public:
    Command::Status execute(Context &context) {
        std::string directory;
        *context.input >> directory;
        context.ftp.writeCmd("CWD " + directory + FTPClient::END_LINE);

        context.ftp.readInto(*context.output);

        std::stringstream currDirStream;
        context.ftp.pwd(currDirStream);
        int pwd_code;
        currDirStream >> pwd_code;
        currDirStream >> context.workingDirectory;
        return OK;
    }
};

class LsCmd : public Command {
public:
    Command::Status execute(Context &context) {
        Socket *dataSocket = context.ftp.openPassive(*context.output);  // FTP server PASV command
        if (dataSocket == NULL) {
            *context.output << "Could not establish data connection." << std::endl;
            return ERROR;
        }
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
        return OK;
    }
};

class GetCmd : public Command {
public:
    Command::Status execute(Context &context) {
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
                return OK;
            } else {
                *context.output << "Could not establish data connection." << std::endl;
                return ERROR;
            }
        } else {
            *context.output << "File error" << std::endl;
            return ERROR;
        }
    }
};

class PutCmd : public Command {
public:
    Command::Status execute(Context &context) {
        std::string localFile;
        std::string remoteFile;
        *context.output << "(local-file) ";
        *context.input >> localFile;
        *context.output << "(remote-file) ";
        *context.input >> remoteFile;
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

class Help : public Command {
    public:
        Command::Status execute(Context &context) {
            *context.output << "Commands:" << std::endl
                            << "open <host> <port> - open new ftp connection" << std::endl
                            << "pwd - print working directory" << std::endl
                            << "ls - list all files in current directory" << std::endl
                            << "cd <directory> - change current working directory to given directory" << std::endl
                            << "mkdir <directory> - create new directory within current directory" << std::endl
                            << "get <filename> - download remote file" << std::endl
                            << "put <filename> - upload local file" << std::endl;
            return OK;
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
    std::auto_ptr<Command> help(new Help());

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
    commands["help"] = help.get();
    Context context(std::cin, std::cout);
    std::string shellName = "ftp";
    std::string commandStr;
    while (1) {
        std::cout << shellName << ":" << context.workingDirectory << "> " << std::flush;
        std::cin >> commandStr;


        std::map<std::string,Command*>::iterator itCmd = commands.find(commandStr);
        if (itCmd == commands.end()) {
            std::cerr << "Command \"" << commandStr << "\" Does Not Exist!" << std::endl;
        } else {
            Command::Status retCode = itCmd->second->execute(context);
            switch (retCode) {
                case Command::ERROR: std::cout << "ERROR!" << std::endl; break;
                case Command::EXIT: std::cout << "exiting..." << std::endl; return 0;
                default: break;
            }
        }
    }

}
