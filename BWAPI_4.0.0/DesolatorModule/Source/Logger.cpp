#include "Logger.h"

Logger::Logger(const std::string & filename, std::ios_base::openmode mode) {
    stream_.open(filename.c_str(), mode);
}

Logger::~Logger() {
    stream_.close();
}


