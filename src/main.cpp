// this is the caretc a compiler for the C^
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
constexpr const char* kProgramName = "caretc";
constexpr const char* kVersion = "caretc 0.1.0 (dev-v0.1.0)";

struct Options {
    bool show_help = false;
    bool show_version = false;
    std::string input_file;
    std::string output_file = "a.out";
    std::vector<std::string> diagnostics;
};

void print_help() {
    std::cout
        << "Usage: " << kProgramName << " [options] <file>\n"
        << "\n"
        << "caretc - the compiler for the C^ programming language.\n"
        << "\n"
        << "Options:\n"
        << "  -h, --help              Show this help message and exit\n"
        << "      --version           Print version information and exit\n"
        << "  -o <file>               Write output to <file> (default: a.out)\n"
        << "  --emit=<stage>          Stop after <stage>: lex|parse|sema|hir|mir|lir|asm\n"
        << "  -v, --verbose           Enable verbose diagnostics\n"
        << "      --target=<triple>   Target triple (e.g. x86_64-linux-gnu)\n"
        << "\n"
        << "Examples:\n"
        << "  " << kProgramName << " hello.cca\n"
        << "  " << kProgramName << " -o hello hello.cca\n"
        << "  " << kProgramName << " --emit=lir hello.cca\n"
        << "\n"
        << "Report bugs to <https://github.com/anomalyco/caret/issues>\n";
}

void print_version() {
    std::cout << kVersion << "\n"
              << "this is the caretc a compiler for the C^\n"
              << "Copyright (c) 2026 The C^ Authors.\n"
              << "Licensed under the MIT License.\n";
}

bool parse_arguments(int argc, char** argv, Options& opts) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            opts.show_help = true;
        } else if (arg == "--version") {
            opts.show_version = true;
        } else if (arg == "-v" || arg == "--verbose") {
            opts.diagnostics.push_back("verbose");
        } else if (arg.rfind("--emit=", 0) == 0) {
            opts.diagnostics.push_back("emit=" + arg.substr(7));
        } else if (arg.rfind("--target=", 0) == 0) {
            opts.diagnostics.push_back("target=" + arg.substr(9));
        } else if (arg == "-o") {
            if (i + 1 >= argc) {
                std::cerr << kProgramName << ": error: missing file argument after '-o'\n";
                return false;
            }
            opts.output_file = argv[++i];
        } else if (!arg.empty() && arg[0] == '-') {
            std::cerr << kProgramName << ": error: unknown option '" << arg << "'\n"
                      << "Try '" << kProgramName << " --help' for more information.\n";
            return false;
        } else {
            if (!opts.input_file.empty()) {
                std::cerr << kProgramName << ": error: multiple input files specified\n";
                return false;
            }
            opts.input_file = arg;
        }
    }
    return true;
}

int run_compile(const Options& opts) {
    if (opts.input_file.empty()) {
        std::cerr << kProgramName << ": error: no input file\n"
                  << "Try '" << kProgramName << " --help' for more information.\n";
        return 1;
    }

    std::error_code ec;
    if (!std::filesystem::exists(opts.input_file, ec)) {
        std::cerr << kProgramName << ": error: cannot find file '" << opts.input_file << "'\n";
        return 1;
    }

    std::ifstream in(opts.input_file);
    if (!in) {
        std::cerr << kProgramName << ": error: cannot open file '" << opts.input_file << "'\n";
        return 1;
    }

    std::ostringstream buffer;
    buffer << in.rdbuf();
    const std::string source = buffer.str();

    std::cout << kProgramName << ": compiling '" << opts.input_file
              << "' -> '" << opts.output_file << "'\n";

    (void)source;
    std::cerr << kProgramName << ": fatal: compiler pipeline not yet implemented\n";
    return 1;
}
}

int main(int argc, char** argv) {
    Options opts;
    if (!parse_arguments(argc, argv, opts)) {
        return 1;
    }
    if (opts.show_help) {
        print_help();
        return 0;
    }
    if (opts.show_version) {
        print_version();
        return 0;
    }
    return run_compile(opts);
}
