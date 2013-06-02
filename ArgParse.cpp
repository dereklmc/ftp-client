#include "ArgParse.h"

#include <sstream>

ArgParse::ArgParse() :
    numRequiredArgs(0)
{
    // ctr
}

void ArgParse::addArgument(std::string arg, bool require) {
    argNames.push_back(arg);
    numRequiredArgs += (int) require;
}

std::string ArgParse::usage(char *programName) const {
    std::stringstream ss;
    ss << programName;
    for (std::vector<std::string>::const_iterator arg = argNames.begin(); arg != argNames.end(); ++arg)
    {
        ss << " <" << *arg << ">";
    }
    return ss.str();
}

bool ArgParse::parse(const int argc, char *argv[]) {
    if (argc < numRequiredArgs) {
        return false;
    }

    for (int i = 0; i < argc; i++) {
        std::string argName = argNames[i];
        argMap[argName] = argv[i + 1];
    }

    return true;
}

char* ArgParse::arg(const std::string& argName) const {
    std::map<std::string,char*>::const_iterator it = argMap.find(argName);
    if (it == argMap.end()) {
        return NULL;
    }
    return it->second;
}