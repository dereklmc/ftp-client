#ifndef __COMMAND__
#define __COMMAND__

#include "Context.h"

/**
 * Interface for commands called by a user on the command line.
 *
 * This is simply a combination of the Command Patter and interpreter pattern.
 */
class Command {

    public:
        /** Return status of execution */
        enum Status {
            OK, ERROR, EXIT
        };

        virtual ~Command() { }

        virtual Command::Status execute(Context &context) =0;
};

#endif