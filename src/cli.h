#pragma once

#include <string>
#include <cstdint>

// Command-line argument configuration
struct CLIConfig {
    // Game settings
    uint32_t seed = 0;              // 0 = random seed
    int difficulty = 1;              // 0=easy, 1=normal, 2=hard
    
    // Display settings
    bool noColor = false;            // Disable ANSI colors
    bool noUnicode = false;          // Use ASCII-only characters
    
    // Debug settings
    bool debug = false;              // Enable debug mode
    std::string logFile;             // Log file path (empty = no logging)
    
    // Control flow
    bool showHelp = false;           // Show help and exit
    bool showVersion = false;        // Show version and exit
    bool exitRequested = false;      // Exit after parsing (help/version/error)
    int exitCode = 0;                // Exit code if exitRequested
};

namespace cli {
    // Parse command-line arguments and return configuration
    CLIConfig parse(int argc, char* argv[]);
    
    // Print help message to stdout
    void print_help(const char* programName);
    
    // Print version info to stdout
    void print_version();
    
    // Get global config (set after parse)
    const CLIConfig& config();
    
    // Initialize global config
    void set_config(const CLIConfig& cfg);
}


