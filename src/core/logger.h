#ifndef CORE_LOGGER_H
#define CORE_LOGGER_H

#include <Arduino.h>

enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR
};

class Logger {
public:
    static void setLevel(LogLevel level) {
        currentLevel = level;
    }
    
    static void debug(const char* message) {
        log(LogLevel::DEBUG, message);
    }
    
    static void info(const char* message) {
        log(LogLevel::INFO, message);
    }
    
    static void warn(const char* message) {
        log(LogLevel::WARN, message);
    }
    
    static void error(const char* message) {
        log(LogLevel::ERROR, message);
    }
    
    static void log(LogLevel level, const char* message) {
        if (level < currentLevel) return;
        
        unsigned long timestamp = millis();
        const char* levelStr = getLevelString(level);
        
        Serial.printf("[%lu] [%s] %s\n", timestamp, levelStr, message);
    }
    
    static void logf(LogLevel level, const char* format, ...) {
        if (level < currentLevel) return;
        
        char buffer[256];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        
        log(level, buffer);
    }
    
private:
    static LogLevel currentLevel;
    
    static const char* getLevelString(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO:  return "INFO";
            case LogLevel::WARN:  return "WARN";
            case LogLevel::ERROR: return "ERROR";
            default: return "UNKNOWN";
        }
    }
};

// Convenience macros
#define LOG_DEBUG(msg) Logger::debug(msg)
#define LOG_INFO(msg) Logger::info(msg)
#define LOG_WARN(msg) Logger::warn(msg)
#define LOG_ERROR(msg) Logger::error(msg)

#define LOG_DEBUGF(fmt, ...) Logger::logf(LogLevel::DEBUG, fmt, __VA_ARGS__)
#define LOG_INFOF(fmt, ...) Logger::logf(LogLevel::INFO, fmt, __VA_ARGS__)
#define LOG_WARNF(fmt, ...) Logger::logf(LogLevel::WARN, fmt, __VA_ARGS__)
#define LOG_ERRORF(fmt, ...) Logger::logf(LogLevel::ERROR, fmt, __VA_ARGS__)

#endif