#include "input.h"
#include "logger.h"

#include <iostream>
#include <algorithm>
#include <chrono>
#include <thread>

#if defined(__unix__) || defined(__APPLE__)
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

static termios orig_termios{};

bool input::enable_raw_mode() {
    if (!isatty(STDIN_FILENO)) {
        return false;
    }
    termios raw{};
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
        return false;
    }
    raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        return false;
    }
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    return true;
}

void input::disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

// Helper to parse escape sequences for arrow keys
// FIXED: Use non-blocking reads with timeout to prevent freezes
static int parse_escape_sequence() {
    char seq[3];
    
    // Read the next character with timeout (non-blocking)
    // Since we're in non-blocking mode, read will return immediately
    ssize_t n = read(STDIN_FILENO, &seq[0], 1);
    if (n != 1) {
        // No data available - incomplete escape sequence, return escape
        return '\033';
    }
    
    // Try to read second character (also non-blocking)
    // Use a small loop with timeout to handle delayed escape sequences
    auto start = std::chrono::steady_clock::now();
    constexpr int timeoutMs = 50; // 50ms timeout for escape sequence
    constexpr int maxIterations = 100; // Safety limit to prevent infinite loops
    int iterations = 0;
    
    while (iterations < maxIterations) {
        iterations++;
    n = read(STDIN_FILENO, &seq[1], 1);
        if (n == 1) {
            // Got the second character
            break;
        }
        
        // Check timeout
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
        if (elapsed.count() > timeoutMs) {
            // Timeout - incomplete escape sequence
            Logger::instance().log(LogLevel::WARN, "Escape sequence timeout after " + std::to_string(elapsed.count()) + "ms");
            return '\033';
        }
        
        // Small sleep to avoid busy-waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    // Safety check: if we hit iteration limit, return escape
    if (iterations >= maxIterations) {
        Logger::instance().log(LogLevel::WARN, "Escape sequence hit iteration limit - possible terminal issue");
        return '\033';
    }
    
    if (seq[0] == '[') {
        switch (seq[1]) {
            case 'A': return input::KEY_UP;
            case 'B': return input::KEY_DOWN;
            case 'C': return input::KEY_RIGHT;
            case 'D': return input::KEY_LEFT;
        }
    }
    return '\033'; // Unknown escape sequence
}

int input::read_key_nonblocking() {
    char c;
    ssize_t n = read(STDIN_FILENO, &c, 1);
    if (n == 1) {
        // Check for escape sequence (arrow keys)
        if (c == '\033') {
            LOG_OP_START("parse_escape_sequence");
            auto parseStart = std::chrono::steady_clock::now();
            int result = parse_escape_sequence();
            auto parseEnd = std::chrono::steady_clock::now();
            auto parseDuration = std::chrono::duration_cast<std::chrono::milliseconds>(parseEnd - parseStart);
            LOG_OP_END("parse_escape_sequence");
            
            // Warn if escape sequence parsing takes too long
            if (parseDuration.count() > 100) {
                LOG_WARN("Escape sequence parsing took " + std::to_string(parseDuration.count()) + "ms - may indicate input issue");
            }
            return result;
        }
        return static_cast<unsigned char>(c);
    }
    return -1;
}

int input::read_key_blocking() {
    LOG_OP_START("read_key_blocking");
    auto startTime = std::chrono::steady_clock::now();
    
    char c;
    int attempts = 0;
    constexpr int MAX_ATTEMPTS = 100000;  // Safety limit
    constexpr int LOG_INTERVAL = 10000;
    constexpr int WARN_INTERVAL = 1000;  // Warn every 1000 attempts (~1 second)
    
    while (attempts < MAX_ATTEMPTS) {
        attempts++;
        
        // Log periodically to detect hangs
        if (attempts % WARN_INTERVAL == 0) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);
            LOG_WARN("read_key_blocking: Still waiting for input (attempt " + std::to_string(attempts) + 
                     ", elapsed: " + std::to_string(elapsed.count()) + "ms)");
        } else if (attempts % LOG_INTERVAL == 0) {
            LOG_DEBUG("read_key_blocking: waiting for input (attempt " + std::to_string(attempts) + ")");
        }
        
        ssize_t n = read(STDIN_FILENO, &c, 1);
        if (n == 1) {
            // Check for escape sequence (arrow keys)
            if (c == '\033') {
                LOG_OP_END("read_key_blocking");
                return parse_escape_sequence();
            }
            LOG_OP_END("read_key_blocking");
            return static_cast<unsigned char>(c);
        }
        
        // Small sleep to avoid busy-waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    LOG_WARN("read_key_blocking: hit attempt limit, returning -1");
    LOG_OP_END("read_key_blocking");
    return -1;
}

input::TerminalSize input::get_terminal_size() {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        // Fallback to default
        return {80, 24};
    }
    return {static_cast<int>(ws.ws_col), static_cast<int>(ws.ws_row)};
}

input::ViewportSize input::calculate_viewport(int term_width, int term_height) {
    // Reserve space for UI: status bar (3), message log (6), borders (4)
    constexpr int ui_width_reserve = 4;   // Left/right borders + padding
    constexpr int ui_height_reserve = 13; // Status + messages + borders
    
    // Calculate available space
    int available_w = term_width - ui_width_reserve;
    int available_h = term_height - ui_height_reserve;
    
    // Clamp to min/max
    constexpr int min_w = 40, max_w = 100;
    constexpr int min_h = 15, max_h = 35;
    
    int vw = std::clamp(available_w, min_w, max_w);
    int vh = std::clamp(available_h, min_h, max_h);
    
    return {vw, vh};
}

#else
// Fallbacks for non-POSIX (e.g., Windows dev machines). Blocking input only.
bool input::enable_raw_mode() {
    return true;
}

void input::disable_raw_mode() {
}

int input::read_key_nonblocking() {
    return -1;
}

int input::read_key_blocking() {
    int c = std::cin.get();
    return c;
}

input::TerminalSize input::get_terminal_size() {
    // Default fallback for non-POSIX
    return {80, 24};
}

input::ViewportSize input::calculate_viewport(int term_width, int term_height) {
    int available_w = term_width - 4;
    int available_h = term_height - 13;
    int vw = std::max(40, std::min(available_w, 100));
    int vh = std::max(15, std::min(available_h, 35));
    return {vw, vh};
}
#endif


