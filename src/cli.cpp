#include "cli.h"
#include <iostream>
#include "logger.h"
#include <cstring>
#include <cstdlib>

static CLIConfig g_config;

namespace cli {

const CLIConfig& config() {
    return g_config;
}

void set_config(const CLIConfig& cfg) {
    g_config = cfg;
}

void print_version() {
    std::cout << "Rogue Depths v1.0.0\n";
    std::cout << "A terminal-based roguelike dungeon crawler\n";
    std::cout << "Built with C++17\n";
}

void print_help(const char* programName) {
    std::cout << "Rogue Depths - Terminal Roguelike\n\n";
    std::cout << "Usage: " << programName << " [OPTIONS]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -h, --help              Show this help message and exit\n";
    std::cout << "  -v, --version           Show version information and exit\n";
    std::cout << "  -s, --seed <number>     Set random seed for dungeon generation\n";
    std::cout << "  -d, --difficulty <lvl>  Set difficulty: easy, normal, hard (default: normal)\n";
    std::cout << "  --debug                 Enable debug mode (shows extra info)\n";
    std::cout << "  --log-file <path>       Write debug log to specified file\n";
    std::cout << "  --no-color              Disable ANSI color output\n";
    std::cout << "  --no-unicode            Use ASCII-only characters (no box-drawing)\n";
    std::cout << "\n";
    std::cout << "Examples:\n";
    std::cout << "  " << programName << " --seed 12345\n";
    std::cout << "  " << programName << " --difficulty hard --debug\n";
    std::cout << "  " << programName << " --no-unicode --no-color\n";
    std::cout << "  " << programName << " --log-file game.log\n";
    std::cout << "\n";
    std::cout << "In-Game Controls:\n";
    std::cout << "  W/A/S/D or Arrows  Move player\n";
    std::cout << "  I                  Toggle inventory\n";
    std::cout << "  TAB                Cycle UI views\n";
    std::cout << "  ?                  Show help\n";
    std::cout << "  Q                  Quit game\n";
}

CLIConfig parse(int argc, char* argv[]) {
    CLIConfig config;
    
    for (int i = 1; i < argc; ++i) {
        const char* arg = argv[i];
        
        // Help
        if (std::strcmp(arg, "-h") == 0 || std::strcmp(arg, "--help") == 0) {
            config.showHelp = true;
            config.exitRequested = true;
            config.exitCode = 0;
            continue;
        }
        
        // Version
        if (std::strcmp(arg, "-v") == 0 || std::strcmp(arg, "--version") == 0) {
            config.showVersion = true;
            config.exitRequested = true;
            config.exitCode = 0;
            continue;
        }
        
        // Seed
        if (std::strcmp(arg, "-s") == 0 || std::strcmp(arg, "--seed") == 0) {
            if (i + 1 < argc) {
                config.seed = static_cast<uint32_t>(std::strtoul(argv[++i], nullptr, 10));
            } else {
                LOG_ERROR("Error: --seed requires a number argument");
                config.exitRequested = true;
                config.exitCode = 1;
            }
            continue;
        }
        
        // Difficulty
        if (std::strcmp(arg, "-d") == 0 || std::strcmp(arg, "--difficulty") == 0) {
            if (i + 1 < argc) {
                const char* lvl = argv[++i];
                if (std::strcmp(lvl, "easy") == 0) {
                    config.difficulty = 0;
                } else if (std::strcmp(lvl, "normal") == 0) {
                    config.difficulty = 1;
                } else if (std::strcmp(lvl, "hard") == 0) {
                    config.difficulty = 2;
                } else {
                    LOG_ERROR(std::string("Error: Invalid difficulty '") + lvl + "'. Use: easy, normal, hard");
                    config.exitRequested = true;
                    config.exitCode = 1;
                }
            } else {
                LOG_ERROR("Error: --difficulty requires an argument");
                config.exitRequested = true;
                config.exitCode = 1;
            }
            continue;
        }
        
        // Debug mode
        if (std::strcmp(arg, "--debug") == 0) {
            config.debug = true;
            continue;
        }
        
        // Log file
        if (std::strcmp(arg, "--log-file") == 0) {
            if (i + 1 < argc) {
                config.logFile = argv[++i];
            } else {
                LOG_ERROR("Error: --log-file requires a path argument");
                config.exitRequested = true;
                config.exitCode = 1;
            }
            continue;
        }
        
        // No color
        if (std::strcmp(arg, "--no-color") == 0) {
            config.noColor = true;
            continue;
        }
        
        // No unicode
        if (std::strcmp(arg, "--no-unicode") == 0) {
            config.noUnicode = true;
            continue;
        }
        
        // Unknown argument
        LOG_ERROR(std::string("Error: Unknown argument '") + arg + "'");
        LOG_ERROR("Use --help for usage information");
        config.exitRequested = true;
        config.exitCode = 1;
    }
    
    return config;
}

} // namespace cli


