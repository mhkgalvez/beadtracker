/* 
 * File:   ReadException.cpp
 * Author: matheus
 * 
 * Created on May 31, 2013, 4:47 PM
 */

#include "ReadException.hpp"

ReadException::ReadException(std::string message) : std::runtime_error(message) {}

ReadException::ReadException(const ReadException& orig) : std::runtime_error(orig.what()) {}


