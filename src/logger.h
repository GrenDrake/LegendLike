#ifndef LOGGER_H
#define LOGGER_H

#include <iosfwd>
#include <string>

struct PHYSFS_File;

class Logger {
public:
    static Logger& getInstance();
    void endLog();

    void log(const std::string &text);
    void error(const std::string &text);
    void warn(const std::string &text);
    void info(const std::string &text);
    void debug(const std::string &text);

    Logger(Logger const&)         = delete;
    void operator=(Logger const&) = delete;
private:
    Logger();

    static Logger *mLoggerInstance;
    PHYSFS_File *mLogFile;
};


#endif
