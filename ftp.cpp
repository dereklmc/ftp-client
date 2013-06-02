// Library
#include <iostream>
#include <map>
#include <memory>  // For auto_ptr, which is deprecated in C++11 (gcc v4.7+)
#include <stdlib.h>

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
            context.ftp = new FTPClient(hostname, port);
            *context.output << context.ftp->read();
        }
};

class QuitCmd : public Command {
    public:
        void execute(Context &context) {
            *context.output << "GOODBYE!" << std::endl;
            exit(0);
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
    CommandParser cmdParser("ftp");
    cmdParser.addCommand("open", open.get());
    cmdParser.addCommand("quit", quit.get());

    Context context(std::cin, std::cout);
    while (1) {
        cmdParser.parse(context);
    }

}