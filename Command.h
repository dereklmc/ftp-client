#ifndef __COMMAND__
#define __COMMAND__

#include "Context.h"

class Command {

    public:
        enum Status {
            OK, ERROR, EXIT
        };

        virtual ~Command() { }

        virtual Command::Status execute(Context &context) =0;
};

#endif