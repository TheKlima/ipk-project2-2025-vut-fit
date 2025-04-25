/**
* @file err.cpp
 * @author Andrii Klymenko
 * @brief Definition of utility function for printing protocol's error messages.
 */

#include "err.h"
#include <iostream>

void printErrMsg(const std::string_view err_msg)
{
    std::cout << "ERROR: " << err_msg << std::endl;
}