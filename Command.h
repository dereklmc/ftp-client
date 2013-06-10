#ifndef __COMMAND__
#define __COMMAND__

#include "Context.h"

class Command {

    public:
        enum STATUS {
            OK, ERROR, EXIT
        };

        virtual ~Command() { }

        virtual Command::STATUS execute(Context &context) =0;
};

#endif