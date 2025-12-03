#include "logger.h"
#include <iostream>

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

Logger::~Logger() {
    shutdown();
}

void Logger::init(const std::string& filePath) {
    if (filePath.empty()) {
        enabled_ = false;
        return;
    }
    
    filePath_ = filePath;
    file_.open(filePath, std::ios::out | std::ios::trunc);
    
    if (file_.is_open()) {
        enabled_ = true;
        info("=== Rogue Depths Log Started ===");
        info("Log file: " + filePath);
    } else {
        enabled_ = false;
        std::cerr << "Warning: Could not open log file: " << filePath << std::endl;
    }
}

void Logger::shutdown() {
    if (enabled_ && file_.is_open()) {
        info("=== Rogue Depths Log Ended ===");
        file_.close();
    }
    enabled_ = false;
}

std::string Logger::timestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    ss << '.' << std::setfill('0') << std::setw(3) << us.count();
    return ss.str();
}

const char* Logger::level_str(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO ";
        case LogLevel::WARN:  return "WARN ";
        case LogLevel::ERROR: return "ERROR";
        default: return "?????";
    }
}

void Logger::log(LogLevel level, const std::string& message) {
    if (!enabled_ || !file_.is_open()) return;
    
    file_ << "[" << timestamp() << "] [" << level_str(level) << "] " << message << "\n";
    file_.flush();  // Ensure immediate write for debugging
    
    // stderr output disabled - all logs still written to file for debugging
    // if (level == LogLevel::ERROR || level == LogLevel::WARN || level == LogLevel::DEBUG) {
    //     std::cerr << "[" << timestamp() << "] [" << level_str(level) << "] " << message << "\n";
    //     std::cerr.flush();
    // }
    // if (level == LogLevel::INFO) {
    //     std::cerr << "[" << timestamp() << "] [" << level_str(level) << "] " << message << "\n";
    //     std::cerr.flush();
    // }
}

void Logger::log_timing(const std::string& operation, long long milliseconds) {
    // IMPROVED: Optimize string building - use reserve and append to avoid multiple allocations with timestamp
    std::string msg;
    msg.reserve(operation.size() + 50);
    msg = "[";
    msg += timestamp();
    msg += "] TIMING: ";
    msg += operation;
    msg += " took ";
    msg += std::to_string(milliseconds);
    msg += "ms";
    
    if (milliseconds > 100) {  // Only log operations taking >100ms
        warn(msg);
    } else {
        debug(msg);
    }
    // stderr output disabled - logs still written to file for debugging
    // std::cerr << msg << std::endl;
    // std::cerr.flush();
}

void Logger::log_operation_start(const std::string& operation) {
    auto startTime = std::chrono::steady_clock::now();
    operation_starts_[operation] = startTime;
    // IMPROVED: Optimize string building with timestamp
    std::string msg;
    msg.reserve(operation.size() + 50);
    msg = "[";
    msg += timestamp();
    msg += "] >>> START: ";
    msg += operation;
    debug(msg);
    // stderr output disabled - logs still written to file for debugging
    // std::cerr << msg << std::endl;
    // std::cerr.flush();
}

void Logger::log_operation_end(const std::string& operation) {
    auto it = operation_starts_.find(operation);
    if (it != operation_starts_.end()) {
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - it->second);
        long long ms = duration.count();
        
        // IMPROVED: Optimize string building - use reserve and append with timestamp
        std::string msg;
        msg.reserve(operation.size() + 50);
        msg = "[";
        msg += timestamp();
        msg += "] <<< END: ";
        msg += operation;
        msg += " (took ";
        msg += std::to_string(ms);
        msg += "ms)";
        debug(msg);
        // stderr output disabled - logs still written to file for debugging
        // std::cerr << msg << std::endl;
        // std::cerr.flush();
        
        if (ms > 500) {
            // IMPROVED: Optimize string building for freeze detection
            std::string freezeMsg;
            freezeMsg.reserve(operation.size() + 70);
            freezeMsg = "[";
            freezeMsg += timestamp();
            freezeMsg += "] FREEZE DETECTED: ";
            freezeMsg += operation;
            freezeMsg += " took ";
            freezeMsg += std::to_string(ms);
            freezeMsg += "ms (>500ms threshold)";
            warn(freezeMsg);
        }
        
        operation_starts_.erase(it);
    } else {
        // IMPROVED: Optimize string building with timestamp
        std::string msg;
        msg.reserve(operation.size() + 55);
        msg = "[";
        msg += timestamp();
        msg += "] <<< END: ";
        msg += operation;
        msg += " (no start time recorded)";
        debug(msg);
        // stderr output disabled - logs still written to file for debugging
        // std::cerr << msg << std::endl;
        // std::cerr.flush();
    }
}


