#pragma once

#include <cstdint>  
#include <iostream>
#include <string>

enum LoggingVerbosity : uint8_t
{
    none = 0,
    error = 1,
    warning = 2,
    verbose = 3
};

static constexpr LoggingVerbosity LOG_VERBOSITY = LoggingVerbosity::verbose;

// macro to print log messages based on verbosity level
// take in a message as a std::string and a verbosity level

#define SCLOG(verbosity, message)          \
    if (LOG_VERBOSITY >= verbosity) {      \
        std::cout << message << std::endl; \
    }

