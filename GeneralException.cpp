#include "GeneralException.hpp"

GeneralException::GeneralException(std::string message) : std::runtime_error(message) {}

GeneralException::GeneralException(const GeneralException& orig) : std::runtime_error(orig.what()) {}
