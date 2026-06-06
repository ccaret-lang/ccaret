// this is the caretc a compiler for the C^
#include "C/c_backend.hpp"
#include "Diagnostics/diagnostics.hpp"
#include "Frontend/Lexer/lexer.hpp"
#include "Frontend/Parser/parser.hpp"
#include "Frontend/Syntax/syntax.hpp"
#include "Frontend/Token/token.hpp"
#include "caretc.hpp"

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {
constexpr const char* kProgramName = "caretc";
constexpr const char* kVersion = "caretc 0.1.0 (dev-v0.1.0)";

struct Options {
    bool show_help = false;
    bool show_version = false;
    bool verbose = false;
    bool keep_c = false;
    bool run_cc = true;          // run the system C compiler to produce binary
    std::string emit_stage;      // lex|parse|c  (default: full pipeline)
    std::string input_file;
    std::string output_file = "a.out";
    std::string target = "x86_64-linux-gnu";
    std::string cc = "cc";       // system C compiler
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
        << "      --emit=<stage>      Stop after <stage>: lex|parse|c\n"
        << "      --no-cc             Emit C only; do not invoke system compiler\n"
        << "      --keep-c            Keep the generated .c file\n"
        << "  -v, --verbose           Enable verbose diagnostics\n"
        << "      --target=<triple>   Target triple (informational)\n"
        << "      --cc=<compiler>     System C compiler to invoke (default: cc)\n"
        << "\n"
        << "Examples:\n"
        << "  " << kProgramName << " hello.cca\n"
        << "  " << kProgramName << " -o hello hello.cca\n"
        << "  " << kProgramName << " --emit=c hello.cca\n"
        << "\n"
        << "Report bugs to https://github.com/ccaret-lang/ccaret\n";
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
            opts.verbose = true;
        } else if (arg == "--no-cc") {
            opts.run_cc = false;
        } else if (arg == "--keep-c") {
            opts.keep_c = true;
        } else if (arg.rfind("--emit=", 0) == 0) {
            opts.emit_stage = arg.substr(7);
        } else if (arg.rfind("--target=", 0) == 0) {
            opts.target = arg.substr(9);
        } else if (arg.rfind("--cc=", 0) == 0) {
            opts.cc = arg.substr(5);
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

    // Diagnostics engine: collects all errors with the C^ error modal format.
    caret::DiagnosticsEngine diags;

    // === Lex ===
    if (opts.verbose) {
        std::cout << "[1/4] lexing...\n";
    }
    std::vector<caret::frontend::Token> tokens =
        caret::frontend::lex(source, opts.input_file);
    if (opts.emit_stage == "lex") {
        for (const auto& t : tokens) {
            std::cout << caret::frontend::token_kind_name(t.kind) << " '"
                      << t.lexeme << "' @"
                      << t.line << ":" << t.column << "\n";
        }
        return 0;
    }

    // === Parse ===
    if (opts.verbose) std::cout << "[2/4] parsing...\n";
    auto parsed = caret::frontend::parse(tokens, diags, opts.input_file);
    if (diags.has_errors()) {
        return diags.render_all(std::cerr, source);
    }
    if (opts.verbose) {
        std::cout << "  parsed " << parsed.unit.size() << " top-level decl(s)\n";
    }
    if (opts.emit_stage == "parse") {
        std::cout << "parse ok: " << parsed.unit.size() << " decl(s)\n";
        return 0;
    }

    // === Analyze (light, optional) ===
    if (opts.verbose) std::cout << "[3/4] semantic analysis (skipped in v0.1.0)\n";

    // === Emit C ===
    if (opts.verbose) std::cout << "[4/4] emitting C...\n";
    auto emitted = caret::backend::c::emit(parsed.unit);
    if (!emitted.ok) {
        return diags.render_all(std::cerr, source);
    }

    // Decide the C file path: beside the input, with .c extension.
    std::filesystem::path src_path(opts.input_file);
    std::filesystem::path c_path =
        src_path.parent_path() /
        (src_path.stem().string() + ".c");

    if (opts.emit_stage == "c") {
        std::cout << emitted.code;
        return 0;
    }

    {
        std::ofstream cf(c_path);
        if (!cf) {
            std::cerr << kProgramName << ": error: cannot write '"
                      << c_path.string() << "'\n";
            return 1;
        }
        cf << emitted.code;
    }
    if (opts.verbose) {
        std::cout << "  wrote " << c_path.string()
                  << " (" << emitted.code.size() << " bytes)\n";
    }

    if (!opts.run_cc) {
        std::cout << kProgramName << ": ok (C emitted, --no-cc)\n";
        return 0;
    }

    // === Invoke system C compiler ===
    std::string cmd = opts.cc + " -O2 -std=c17 -o " + opts.output_file + " " +
                      c_path.string();
    if (opts.verbose) std::cout << "  $ " << cmd << "\n";
    int rc = std::system(cmd.c_str());
    if (rc != 0) {
        std::cerr << kProgramName << ": error: system C compiler failed (exit "
                  << rc << ")\n";
        return 1;
    }
    if (!opts.keep_c) {
        std::error_code rmec;
        std::filesystem::remove(c_path, rmec);
    }
    std::cout << kProgramName << ": ok -> '" << opts.output_file << "'\n";
    return 0;
}
}  // namespace

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
