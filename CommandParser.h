#ifndef __COMMAND_PARSER__
#define __COMMAND_PARSER__

#include <map>

#include "Command.h"
#include "Context.h"

class CommandParser {
    public:
        CommandParser(const std::string shellName);

        bool addCommand(std::string name, Command *cmd);
        void parse(Context &context);

    private:
        const std::string shellName;
        std::map<std::string,Command*> commands;
};

#endif