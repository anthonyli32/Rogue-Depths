#pragma once

namespace input {
    bool enable_raw_mode();
    void disable_raw_mode();

    // Returns -1 if no key available (non-blocking)
    int read_key_nonblocking();

    // Blocks until a key is read
    int read_key_blocking();

    // Special key codes for arrow keys (returned by read_key_*)
    constexpr int KEY_UP    = 1000;
    constexpr int KEY_DOWN  = 1001;
    constexpr int KEY_LEFT  = 1002;
    constexpr int KEY_RIGHT = 1003;

    // Terminal size detection
    struct TerminalSize {
        int width;
        int height;
    };
    
    // Get current terminal dimensions
    TerminalSize get_terminal_size();
    
    // Calculate optimal viewport based on terminal size
    struct ViewportSize {
        int width;
        int height;
    };
    ViewportSize calculate_viewport(int term_width, int term_height);
}


