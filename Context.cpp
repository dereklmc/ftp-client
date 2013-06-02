#include "Context.h"

Context::Context(std::istream &input, std::ostream &output, FTPClient *ftp) :
    input(&input),
    output(&output),
    ftp(ftp)
{
    // ctr
}