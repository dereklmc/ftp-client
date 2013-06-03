/**
 * @file Context.h
 * @brief Representation of external state that a command is applied to.
 *
 * The external state includes all interfaces to external dependencies of the
 * program, such as:
 *
 *  - I/O streams
 *  - Network Connections
 *  - Configuration
 *
 * Currently, context manages two different concerns: I/O and FTP connections.
 * While these should be in separate contexts, sacrificing flexibility allows
 * the current design to simply meet the requirements. Future iterations should
 * implement multiple context handling.
 *
 * @author Mitch Carlson, Derek McLean
 * @version 1.0
 */


#ifndef __CONTEXT__
#define __CONTEXT__

#include <iostream>

#include "FTPClient.h"

/** External context of commands */
class Context {
    public:

        /**
         * Constructor.
         *
         * Creates context from external state variables.
         *
         * @param input - reference to external input stream (e.g. std::cin)
         * @param output - reference to external ouput stream (e.g. std::cout)
         * @param FTPClient - represents a network connection to an ftp server
         */
        Context(std::istream &input, std::ostream &output);

        std::istream *input;  //!< ptr to input stream of external context
        std::ostream *output;  //!< ptr to output stream of external context
        FTPClient ftp;  //!< ptr to ftp network connection in context
};

#endif