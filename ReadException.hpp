/* 
 * File:   ReadException.hpp
 * Author: matheus
 *
 * Created on May 31, 2013, 4:47 PM
 */

#ifndef READEXCEPTION_HPP
#define	READEXCEPTION_HPP

#include <stdexcept>
#include <string>

class ReadException : public std::runtime_error {
public:
    ReadException(std::string message);
    ReadException(const ReadException& orig);
};

#endif	/* READEXCEPTION_HPP */

