/**
 * @file exception.cpp
 * @author Andrii Klymenko
 * @brief Implementation of the custom exception class.
 */

#include "exception.h"

Exception::Exception(std::string_view explanation)
    :
    m_explanation{explanation}
{

}

const char* Exception::what() const noexcept
{
    return m_explanation.c_str();
}

bool Exception::isSigintOrEofReceived() const
{
    return m_explanation.empty();
}
