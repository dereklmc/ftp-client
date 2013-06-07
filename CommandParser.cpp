#include "CommandParser.h"

#include <iostream>

CommandParser::CommandParser(const std::string shellName) :
    shellName(shellName)
{
    // ctr
}

bool CommandParser::addCommand(std::string name, Command *cmd) {
    commands[name] = cmd;
    return true;
}

void CommandParser::parse(Context &context) {
    std::string commandStr;
    *context.output << shellName << ":" << context.workingDirectory << "> " << std::flush;
    *context.input >> commandStr;


    std::map<std::string,Command*>::iterator itCmd = commands.find(commandStr);
    if (itCmd == commands.end()) {
        *context.output << "Command Does Not Exist!" << std::endl;
    } else {
        itCmd->second->execute(context);
    }
}