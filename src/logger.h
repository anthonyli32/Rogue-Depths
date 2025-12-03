#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <map>

enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR
};

class Logger {
public:
    // Singleton access
    static Logger& instance();
    
    // Initialize logger with file path (empty = disabled)
    void init(const std::string& filePath);
    
    // Shutdown and close file
    void shutdown();
    
    // Check if logging is enabled
    bool is_enabled() const { return enabled_; }
    
    // Log a message at the specified level
    void log(LogLevel level, const std::string& message);
    
    // Convenience methods
    void debug(const std::string& message) { log(LogLevel::DEBUG, message); }
    void info(const std::string& message) { log(LogLevel::INFO, message); }
    void warn(const std::string& message) { log(LogLevel::WARN, message); }
    void error(const std::string& message) { log(LogLevel::ERROR, message); }
    
    // Performance timing helpers
    void log_timing(const std::string& operation, long long milliseconds);
    void log_operation_start(const std::string& operation);
    void log_operation_end(const std::string& operation);
    
    // Format helpers for logging with context
    template<typename... Args>
    void debug_fmt(const char* fmt, Args... args) {
        if (enabled_) debug(format(fmt, args...));
    }
    
    template<typename... Args>
    void info_fmt(const char* fmt, Args... args) {
        if (enabled_) info(format(fmt, args...));
    }
    
    template<typename... Args>
    void warn_fmt(const char* fmt, Args... args) {
        if (enabled_) warn(format(fmt, args...));
    }
    
    template<typename... Args>
    void error_fmt(const char* fmt, Args... args) {
        if (enabled_) error(format(fmt, args...));
    }

private:
    Logger() = default;
    ~Logger();
    
    // Prevent copying
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    // Get timestamp string
    std::string timestamp() const;
    
    // Get level string
    const char* level_str(LogLevel level) const;
    
    // Simple format helper (replaces {} with args)
    template<typename T>
    std::string format_impl(std::stringstream& ss, const char* fmt, T value) {
        while (*fmt) {
            if (*fmt == '{' && *(fmt + 1) == '}') {
                ss << value;
                return ss.str() + std::string(fmt + 2);
            }
            ss << *fmt++;
        }
        return ss.str();
    }
    
    template<typename T, typename... Args>
    std::string format_impl(std::stringstream& ss, const char* fmt, T value, Args... args) {
        while (*fmt) {
            if (*fmt == '{' && *(fmt + 1) == '}') {
                ss << value;
                return format_impl(ss, fmt + 2, args...);
            }
            ss << *fmt++;
        }
        return ss.str();
    }
    
    template<typename... Args>
    std::string format(const char* fmt, Args... args) {
        std::stringstream ss;
        return format_impl(ss, fmt, args...);
    }
    
    std::string format(const char* fmt) {
        return std::string(fmt);
    }
    
    bool enabled_ = false;
    std::ofstream file_;
    std::string filePath_;
    
    // Timing tracking for freeze detection
    std::map<std::string, std::chrono::steady_clock::time_point> operation_starts_;
};

// Convenience macro for logging with file/line info in debug builds
#define LOG_DEBUG(msg) Logger::instance().debug(msg)
#define LOG_INFO(msg) Logger::instance().info(msg)
#define LOG_WARN(msg) Logger::instance().warn(msg)
#define LOG_ERROR(msg) Logger::instance().error(msg)

// Performance and timing macros
#define LOG_TIMING(op, ms) Logger::instance().log_timing(op, ms)
#define LOG_OP_START(op) Logger::instance().log_operation_start(op)
#define LOG_OP_END(op) Logger::instance().log_operation_end(op)


