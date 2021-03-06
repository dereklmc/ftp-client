/**
 * @file main.cpp
 * @brief Main
 *
 * @author Derek McLean, Mitch Carlson
 *
 * @date 11-06-2013
 */

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
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>

class OpenCmd : public Command {
    public:
        Command::Status execute(Context &context) {
            std::string hostname;
            int port;
            *context.input >> hostname >> port;
            return open(hostname, port, context);
        }

        Command::Status open(std::string hostname,
                                        int port,
                                Context &context) {
            *context.output << "Open connection to \"" << hostname << ":"
                << port << "\"" << std::endl;
            context.ftp.open(hostname, port);
            context.ftp.readInto(*context.output);

            Command::Status authResult = authorize(context);
            if (authResult != OK) {
                return authResult;
            }

            std::stringstream currDirStream;
            if (!context.ftp.pwd(currDirStream)) {
                return ERROR;
            }
            int pwd_code;
            currDirStream >> pwd_code;
            currDirStream >> context.workingDirectory;
            return OK;
        }

        /** Authorization
         *
         * Prompts user for username and password. Then sends to ftp server
         * for verification.
         *
         * @param context - Input/Output and ftp command socket object
         */
        Command::Status authorize(Context &context) {
            struct passwd *pass;
            pass = getpwuid(getuid());
            std::string netid(pass->pw_name);
            std::string input;

            /* Prompt for Username */
            *context.output << "Name ("
                            << context.ftp.getHostname()
                            << ":" << netid << "): ";
            *context.input >> input;
            if (!context.ftp.writeCmd("USER " + input + FTPClient::END_LINE,
                *context.output)) {
                return ERROR;
            }

            /* Promt for Password */
            std::cout << "Password: ";
            *context.input >> input;
            if (!context.ftp.writeCmd("PASS " + input + FTPClient::END_LINE,
                *context.output)) {
                return ERROR;
            }
            return OK;
        }
};

class PWDCmd : public Command {
    public:
        Command::Status execute(Context &context) {
            if (!context.ftp.pwd(*context.output)) {
                return ERROR;
            }
            return OK;
        }
};

/**
 * Close current connection, if any to the ftp server.
 */
class CloseCmd : public Command {
    public:
        Command::Status execute(Context &context) {
            if (context.ftp.isOpen()) {
                context.ftp.close(*context.output);
                return OK;
            } else {
                *context.output << "No connection to close." << std::endl;
                return ERROR;
            }
        }
};

/**
 * Closes current connection to the ftp server and exits the running program.
 */
class QuitCmd : public CloseCmd {
    public:
        Command::Status execute(Context &context) {
            CloseCmd::execute(context);
            *context.output << "GOODBYE!" << std::endl;
            //exit(0);
            return EXIT;
        }
};

/**
 * Create a new directory on the ftp server.
 */
class MkdirCmd : public Command {
    public:
        Command::Status execute(Context &context) {
            if (!context.ftp.isOpen()) {
                return ERROR;
            }
            std::string dir;
            *context.input >> dir;
            if (!context.ftp.writeCmd("MKD " + dir + FTPClient::END_LINE,
                *context.output)) {
                return ERROR;
            }
            return OK;
        }
};

/**
 * Rename a file on the ftp server.
 *
 * Just changes the name, not the contents; so, the file does not need to be
 * downloaded.
 *
 * TODO: handle status codes returned between rename from and rename to.
 */
class MoveCmd : public Command {
    public:
        Command::Status execute(Context &context) {
            if (!context.ftp.isOpen()) {
                return ERROR;
            }
            std::string origin, dest;
            *context.input >> origin >> dest;
            std::stringstream moveCmdStream;
            moveCmdStream << "RNFR " << origin << FTPClient::END_LINE
                          << "RNTO " << dest << FTPClient::END_LINE;
            if (!context.ftp.writeCmd(moveCmdStream.str(), *context.output)) {
                return ERROR;
            }
            return OK;
        }
};

/**
 * Change the current working directory in the ftp server.
 *
 * This updates the current working directory in the locale console prompt.
 */
class CdCmd : public Command {
public:
    Command::Status execute(Context &context) {
        std::string directory;
        *context.input >> directory;
        if (!context.ftp.writeCmd("CWD " + directory + FTPClient::END_LINE,
            *context.output)) {
            return ERROR;
        }

        std::stringstream currDirStream;
        if (!context.ftp.pwd(currDirStream)) {
            return ERROR;
        }
        int pwd_code;
        currDirStream >> pwd_code;
        currDirStream >> context.workingDirectory;
        return OK;
    }
};

class LsCmd : public Command {
public:
    /**
     * List all files and folders in current directory
     *
     * @param context - General input/output and ftp command socket object
     */
    Command::Status execute(Context &context) {
        Socket *dataSocket = context.ftp.openPassive(*context.output); // PASV

        if (dataSocket == NULL) {
            *context.output << "Could not establish data connection." <<
                std::endl;
            return ERROR;
        }

        /* Fork */
        pid_t pid;
        if((pid = fork()) == -1) {              // fork and check for error
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
            if (!context.ftp.writeCmd("LIST" + FTPClient::END_LINE,
                *context.output)) {
                return ERROR;
            }
        }
        delete dataSocket;
        dataSocket = NULL;
        return OK;
    }
};

class GetCmd : public Command {
public:
    /**
     * Get file from ftp server
     *
     * @param context - prompts user for input, takes file name of file to get
     */
    Command::Status execute(Context &context) {
        struct timeval startTime,endTime;    // to record time to retrieve file
        std::string fileName;
        *context.input >> fileName;

        // permission flags = read user | write user | read group | read other
        mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
                                // write only | create
        if(open( fileName.c_str(), O_WRONLY | O_CREAT, mode )) {

            /* Open dedicated data socket */
            Socket *dataSocket = context.ftp.openPassive(*context.output);
            if (dataSocket == NULL){            // error check
                *context.output << "Could not establish data connection." <<
                    std::endl;
                return ERROR;
            }
            /* Fork process */
            pid_t pid;
            if((pid = fork()) == -1) {          // fork
                perror("fork error");
                exit(EXIT_FAILURE);
            }
            /* Block on read() */
            else if (pid == 0) {                // child process
                std::fstream file;              // create file from in-stream
                file.open(fileName.c_str(), std::fstream::in |
                    std::fstream::out);
                dataSocket->readInto(file);
                dataSocket->shutdown();
                file.close();
                exit(EXIT_SUCCESS);
            }

            /* Retrieve file from server */
            else {                                      // parent process
                if (!context.ftp.writeCmd("TYPE I" + FTPClient::END_LINE,
                    *context.output)) {                 // binary file transfer
                    return ERROR;
                }
                gettimeofday(&startTime,NULL);          // start timer
                context.ftp.writeCmd("RETR " + fileName +
                    FTPClient::END_LINE, *context.output);
                int status;
                waitpid(pid,&status,0);                 // wait for child
                gettimeofday(&endTime,NULL);            // end timer
                context.ftp.readInto(*context.output);  // get reply
            }

            delete dataSocket;
            dataSocket = NULL;

            /* Calculate time to get file */
            double dt = ((double)(endTime.tv_sec - startTime.tv_sec) +
                (double)(endTime.tv_usec - startTime.tv_usec)/1000000);
            *context.output << "Received in " << dt <<
                " seconds" << std::endl;
            return OK;
        } else {
            *context.output << "File error" << std::endl;
            return ERROR;
        }
    }
};

class PutCmd : public Command {
public:
    /**
     * Puts a file on the server
     *
     * @param context - prompts user for input, takes file name of a file to
     * put on the server and name it should be stored as.
     */
    Command::Status execute(Context &context) {
        std::string localFile;
        std::string remoteFile;

        /* Check for user input on command line */
        char c = context.input->peek();
        if(c == '\n') {                   // no command line input, prompt user
            *context.output << "(local-file) ";
            *context.input >> localFile;
            *context.output << "(remote-file) ";
            *context.input >> remoteFile;
        } else {                          // get input from command line
            *context.input >> localFile;
            *context.input >> remoteFile;
        }

        // permission flags = read user | write user | read group | read other
        mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
        int dirfd = open( remoteFile.c_str(), O_DIRECTORY | O_RDONLY, mode);
        fchdir( dirfd );

        /* Open dedicated data socket */
        Socket *dataSocket = context.ftp.openPassive(*context.output);
        if (dataSocket == NULL) {
            *context.output << "Could not establish data connection." <<
                std::endl;
            return ERROR;
        }

        /* Fork process */
        pid_t pid;
        if((pid = fork()) == -1) {       // verify fork
            perror("fork error");
            exit(EXIT_FAILURE);
        }

        /* Block on read() */
        else if (pid == 0) {             // child process
            std::fstream file;           // open a stream from a file
            file.open(localFile.c_str(), std::fstream::in |
                std::fstream::out);
            dataSocket->writeFrom(file); // write file to server from data sock
            dataSocket->shutdown();      // notify server that file is done
            file.close();
            exit(EXIT_SUCCESS);
        }

        /* Tell the ftp server to store the file */
        else {                                      // parent process
            if (!context.ftp.writeCmd("TYPE I" + FTPClient::END_LINE,
                *context.output)) {
                return ERROR;
            }
            if (!context.ftp.writeCmd("STOR " + remoteFile +
                FTPClient::END_LINE, *context.output)) {
                return ERROR;
            }
            int status = 0;
            waitpid(pid,&status,0);
            context.ftp.readInto(*context.output);
        }
        delete dataSocket;
        dataSocket = NULL;
        return OK;
    }
};

/**
 * Prints help text.
 *
 * Currently prints all available commands and their usage.
 *
 * TODO: print server help text.
 * TODO: commands register help text with interpreter.
 */
class Help : public Command {
    public:
        Command::Status execute(Context &context) {
            *context.output << "Commands:" << std::endl
                            << "open <host> <port> - open new ftp connection"
                            << std::endl
                            << "pwd - print working directory" << std::endl
                            << "ls - list all files in current directory"
                            << std::endl
                            << "cd <directory> - change current working "
                            << "directory to given directory" << std::endl
                            << "mkdir <directory> - create new directory "
                            << "within current directory" << std::endl
                            << "move <origin> <dest> - rename a file" << std::endl
                            << "get <filename> - download remote file"
                            << std::endl
                            << "put <filename> - upload local file"
                            << std::endl;
            return OK;
        }
};

int main(int argc, char *argv[]) {

    std::string hostname = "";
    if (argc > 1) {
        std::string arg1 = argv[1];
        if (arg1 == "help") {
            std::cout << "usage: " << argv[0] << "[ftpserver]" << std::endl;
        } else {
            hostname = std::string(arg1);
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
    std::auto_ptr<Command> move(new MoveCmd());

    // Map of commands to console text invocations.
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
    commands["move"] = move.get();
    commands["mv"] = move.get();

    Context context(std::cin, std::cout);
    if (hostname.size() > 0) {
        OpenCmd().open(hostname, FTPClient::DEFAULT_PORT, context);
    }
    std::string shellName = "ftp";
    std::string commandStr;
    while (1) {
        std::cout << shellName << ":" << context.workingDirectory << "> "
            << std::flush;
        std::cin >> commandStr;


        std::map<std::string,Command*>::iterator itCmd =
            commands.find(commandStr);
        if (itCmd == commands.end()) {
            std::cerr << "Command \"" << commandStr << "\" Does Not Exist!"
                << std::endl;
        } else {
            Command::Status retCode = itCmd->second->execute(context);
            switch (retCode) {
                case Command::ERROR: std::cout << "ERROR!" << std::endl; break;
                case Command::EXIT: std::cout << "exiting..." << std::endl;
                    return 0;
                default: break;
            }
        }
    }
}
