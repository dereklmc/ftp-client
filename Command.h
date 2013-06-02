#ifndef __COMMAND__
#define __COMMAND__

#include "Context.h"

class Command {

    public:
        virtual ~Command() { }

        virtual void execute(Context &context) =0;

};

#endif