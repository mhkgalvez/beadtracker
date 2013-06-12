/* 
 * File:   GeneralException.hpp
 * Author: matheus
 *
 * Created on June 3, 2013, 12:19 PM
 */

#ifndef GENERALEXCEPTION_HPP
#define	GENERALEXCEPTION_HPP

#include <stdexcept>
#include <string>

class GeneralException : public std::runtime_error {
public:
    GeneralException(std::string message);
    GeneralException(const GeneralException& orig);
};

#endif	/* GENERALEXCEPTION_HPP */

