/* 
 * File:   VideoNotFoundException.hpp
 * Author: matheus
 *
 * Created on May 31, 2013, 11:42 AM
 */

#ifndef OPENVIDEOEXCEPTION_HPP
#define	OPENVIDEOEXCEPTION_HPP

#include <stdexcept>
#include <string>

class OpenVideoException : public std::runtime_error {
public:
    OpenVideoException(std::string message);
    OpenVideoException(const OpenVideoException& orig);
};

#endif	/* VIDEONOTFOUNDEXCEPTION_HPP */

