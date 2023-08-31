#pragma once

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>


template <typename T> T square(const T t) { return t * t; }


class Logger {
public:
    explicit Logger(int log_level, const bool newline=true) :
            _log_level(log_level),
            _newline(newline),
            _oss() { }
    template <typename T>
    Logger& operator<< (const T& output) {
        if (_log_level <= LOG_LEVEL) {
            _oss << output << ' ';
        }
        return *this;
    }

    template <typename T>
    Logger& operator<< (const T* output) {
        if (_log_level <= LOG_LEVEL) {
            _oss << output << ' ';
        }
        return *this;
    }


    Logger& operator<< (decltype(std::left)& output) {
        if (_log_level <= LOG_LEVEL) {
            _oss << output << ' ';
        }
        return *this;
    }

    ~Logger() {
        if (_log_level > LOG_LEVEL) return;
        std::cout << _oss.str();
        if (_newline) {
            std::cout << std::endl;
        } else {
            std::cout << ' ';
        }
    }

    static int LOG_LEVEL;

private:
    int _log_level;
    bool _newline;
    std::ostringstream _oss;
};


#define VLOG(X) Logger(X, true)
