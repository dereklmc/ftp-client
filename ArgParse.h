#ifndef __ARGPARSE__
#define __ARGPARSE__

#include <map>
#include <vector>
#include <string>

class ArgParse {

    public:
        ArgParse();

        void addArgument(std::string arg, bool require=true);
        std::string usage(char *programName) const;
        bool parse(const int argc, char *argv[]);
        char* arg(const std::string& argName) const;

    private:
        std::map<std::string,char*> argMap;
        std::vector<std::string> argNames;

        int numRequiredArgs;
};

#endif