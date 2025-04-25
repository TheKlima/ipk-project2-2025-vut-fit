/**
 * @file exception.h
 * @author Andrii Klymenko <xklyme00>
 * @brief Definition of the class representing program's exception
 */

#ifndef EXCEPTION_H
#define EXCEPTION_H

#include "client.h"
#include <exception>
#include <string>

/**
 * @brief Class representing custom program's exception
 */
class Exception : public std::exception {
private:
    const std::string m_explanation{}; // exception's explanation

public:
    /**
     * @brief Constructs an object (assigns some value to its private member m_explanation)
     *
     * @param explanation exception's cause
     */
    Exception(std::string_view explanation);

    /**
     * @brief Returns a pointer to the object's private member m_explanation
     *
     * @return string containing exception's cause
     */
    const char* what() const noexcept override;


    bool isSigintOrEofReceived() const;
};

#endif // EXCEPTION_H
