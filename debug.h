/**
 * @file DEBUG.h
 * @brief Debug macros.
 *
 * @author Derek McLean, Mitch Carlson
 *
 * @date 11-06-2013
 */

#ifndef __DEBUG__
#define __DEBUG__

// Uncomment to enable debug output.
// #define DEBUG_ENABLE

#ifdef DEBUG_ENABLE
#define DEBUG(out) out
#else
#define DEBUG(out)
#endif

#endif