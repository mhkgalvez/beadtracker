/* 
 * File:   VideoNotFoundException.cpp
 * Author: matheus
 * 
 * Created on May 31, 2013, 11:42 AM
 */

#include "OpenVideoException.hpp"

OpenVideoException::OpenVideoException(std::string message) : std::runtime_error(message) { }

OpenVideoException::OpenVideoException(const OpenVideoException& orig) : std::runtime_error(orig.what()) {
}
