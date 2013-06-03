/**
 * @file Context.cpp
 * @brief Definition of functions in Context.h
 *
 * Currently, defines a constructor for Context to ease initialization of a
 * context object.
 *
 * @author Mitch Carlson, Derek McLean
 * @version 1.0
 */

#include "Context.h"

/**
 * Constructor.
 *
 * Creates context from external state variables.
 *
 * @param input - reference to external input stream (e.g. std::cin)
 * @param output - reference to external ouput stream (e.g. std::cout)
 * @param FTPClient - represents a network connection to an ftp server
 */
Context::Context(std::istream &input, std::ostream &output) :
    input(&input),
    output(&output)
{
    // ctr
}