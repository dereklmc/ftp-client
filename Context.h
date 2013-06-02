#ifndef __CONTEXT__
#define __CONTEXT__

#include <iostream>

#include "FTPClient.h"

class Context {
    public:
        Context(std::istream &input, std::ostream &output, FTPClient *ftp=NULL);

        std::istream *input;
        std::ostream *output;
        FTPClient *ftp;
};

#endif