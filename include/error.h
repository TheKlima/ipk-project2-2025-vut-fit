/**
 * @file err.h
 * @author Andrii Klymenko
 * @brief Declaration of utility function for printing protocol's error messages.
 */

#ifndef ERR_H
#define ERR_H

#include <string_view>

/**
 * @brief Prints a formatted protocol's error message to stdout.
 *
 * @param err_msg The error message to be printed.
 */
void printErrMsg(const std::string_view err_msg);

#endif // ERR_H
