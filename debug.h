#ifndef __DEBUG__
#define __DEBUG__

#define DEBUG_ENABLE

#ifdef DEBUG_ENABLE
#define DEBUG(out) out
#else
#define DEBUG(out)
#endif

#endif