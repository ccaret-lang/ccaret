// caretc — driver for the C^ programming language compiler.
//
// Pipeline (v0.1.0): source -> lex -> parse -> (semantic pass: stub) -> emit C
// -> system cc. The pipeline is intentionally linear; every stage hands its
// output to the next with no shared mutable state besides the diagnostics
// engine. Stop early with --emit=<stage>; skip cc entirely with --no-cc.
//
// Exit codes:
//   0  success
//   1  any of: usage error, IO error, parse/semantic error, cc failure
//
// All user-visible diagnostics flow through caret::DiagnosticsEngine so we
// keep one rendering path (see Diagnostics/diagnostics.cpp and
// syntax/errors_Design.md for the format spec).
#include "C/c_backend.hpp"
#include "Diagnostics/diagnostics.hpp"
#include "Frontend/Lexer/lexer.hpp"
#include "Frontend/Parser/parser.hpp"
// #include "Frontend/Syntax/syntax.hpp"
#include "Frontend/Token/token.hpp"
// #include "caretc.hpp"

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
// kProgramName is the argv[0]-style identity we print in error messages.
// Kept independent of argv[0] so a renamed/symlinked binary still reports
// "caretc" — the canonical name a user sees in --help and bug reports.
constexpr const char *kProgramName = "caretc";
constexpr const char *kVersion = "caretc 0.1.0 (dev-v0.1.0)";

// Aggregates everything parsed from argv before the compile step runs.
// Defaults match the documented behaviour in --help; any change here must
// be reflected there and in the README.
struct Options {
  bool show_help = false;
  bool show_version = false;
  bool verbose = false;   // -v: print per-stage progress to stdout
  bool keep_c = false;    // --keep-c: do not delete the intermediate .c file
  bool run_cc = true;     // false when --no-cc is given (emit C, then stop)
  std::string emit_stage; // "lex" | "parse" | "c"; empty = full pipeline
  std::string input_file; // exactly one .cca source path
  std::string output_file = "a.out";       // final binary path
  std::string target = "x86_64-linux-gnu"; // informational only in v0.1.0
  std::string cc = "cc";                   // system C compiler to invoke
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
      << "      --no-cc             Emit C only; do not invoke system "
         "compiler\n"
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
            << "the official compiler for the C^ programming language\n"
            << "Copyright (c) 2026 The C^ Authors.\n"
            << "Licensed under the MIT License.\n";
}

// Parse argv into Options. Returns false on any usage error (already
// reported to stderr); the caller should exit 1 without printing more.
//
// Recognised forms:
//   short flags:       -h, -v, -o <file>
//   long flags:        --help, --version, --verbose, --keep-c, --no-cc
//   key=value flags:   --emit=<stage>, --target=<triple>, --cc=<compiler>
//
// A single positional argument is taken as the input file; supplying
// more than one is rejected to keep the contract obvious.
bool parse_arguments(int argc, char **argv, Options &opts) {
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
      // rfind(..., 0) == 0 is the idiomatic "starts_with" before C++20.
      opts.emit_stage = arg.substr(7);
    } else if (arg.rfind("--target=", 0) == 0) {
      opts.target = arg.substr(9);
    } else if (arg.rfind("--cc=", 0) == 0) {
      opts.cc = arg.substr(5);
    } else if (arg == "-o") {
      // -o takes a mandatory value in the next argv slot.
      if (i + 1 >= argc) {
        std::cerr << kProgramName
                  << ": error: missing file argument after '-o'\n";
        return false;
      }
      opts.output_file = argv[++i];
    } else if (!arg.empty() && arg[0] == '-') {
      // Anything else starting with '-' is an unknown option. Reject early
      // rather than silently treating it as an input file.
      std::cerr << kProgramName << ": error: unknown option '" << arg << "'\n"
                << "Try '" << kProgramName
                << " --help' for more information.\n";
      return false;
    } else {
      if (!opts.input_file.empty()) {
        std::cerr << kProgramName
                  << ": error: multiple input files specified\n";
        return false;
      }
      opts.input_file = arg;
    }
  }
  return true;
}

// Drive one end-to-end compilation. The flow is:
//   1. validate the input file exists and is readable,
//   2. slurp it into memory (sources are expected to fit in RAM),
//   3. run lex / parse / (semantic — stub) / emit-C,
//   4. optionally hand the generated C off to the system compiler.
//
// Each stage may short-circuit when --emit matches. Diagnostics are
// rendered once at the first stage that flags errors; we do not try to
// continue past a failed parse.
int run_compile(const Options &opts) {
  if (opts.input_file.empty()) {
    std::cerr << kProgramName << ": error: no input file\n"
              << "Try '" << kProgramName << " --help' for more information.\n";
    return 1;
  }

  std::error_code ec;
  if (!std::filesystem::exists(opts.input_file, ec)) {
    std::cerr << kProgramName << ": error: cannot find file '"
              << opts.input_file << "'\n";
    return 1;
  }

  std::ifstream in(opts.input_file);
  if (!in) {
    std::cerr << kProgramName << ": error: cannot open file '"
              << opts.input_file << "'\n";
    return 1;
  }

  // Slurp the entire source into a std::string. The lexer and diagnostic
  // renderer both need random access to the source buffer (the latter to
  // print the offending line under the carets).
  std::ostringstream buffer;
  buffer << in.rdbuf();
  const std::string source = buffer.str();

  std::cout << kProgramName << ": compiling '" << opts.input_file << "' -> '"
            << opts.output_file << "'\n";

  // The diagnostics engine collects every error and warning produced by
  // every stage and renders them at the end in the C^ format (see
  // syntax/errors_Design.md). One engine instance, one render pass.
  caret::DiagnosticsEngine diags;

  // === Stage 1/4: lexing ===
  // Cheap, infallible by design: unknown bytes become TokenKind::Unknown
  // and surface as parse errors later, where we have richer context.
  if (opts.verbose) {
    std::cout << "[1/4] lexing...\n";
  }
  std::vector<caret::frontend::Token> tokens =
      caret::frontend::lex(source, opts.input_file);
  if (opts.emit_stage == "lex") {
    // Debug aid: dump the token stream and stop. Format mirrors the
    // internal Token layout (kind, lexeme, line:col).
    for (const auto &t : tokens) {
      std::cout << caret::frontend::token_kind_name(t.kind) << " '" << t.lexeme
                << "' @" << t.line << ":" << t.column << "\n";
    }
    return 0;
  }

  // === Stage 2/4: parsing ===
  // The parser drives a recursive-descent walk over the token stream and
  // produces an ast::TranslationUnit. Errors are accumulated in `diags`;
  // we bail on the first error pass rather than emit garbage C.
  if (opts.verbose)
    std::cout << "[2/4] parsing...\n";
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

  // === Stage 3/4: semantic analysis ===
  // Placeholder. The full type checker, mutability checker, and reference
  // borrow checker live behind this hook (see Frontend/Semantics). v0.1.0
  // ships without it; the C backend accepts what the parser produces.
  if (opts.verbose)
    std::cout << "[3/4] semantic analysis (skipped in v0.1.0)\n";

  // === Stage 4/4: emit C ===
  // The C backend lowers the AST to a self-contained .c file that the
  // system compiler can build with -std=c17 and no extra flags.
  if (opts.verbose)
    std::cout << "[4/4] emitting C...\n";
  auto emitted = caret::backend::c::emit(parsed.unit);
  if (!emitted.ok) {
    return diags.render_all(std::cerr, source);
  }

  // The generated C file sits next to the source, sharing its stem and
  // gaining a `.c` extension. Example: foo/bar.cca -> foo/bar.c.
  std::filesystem::path src_path(opts.input_file);
  std::filesystem::path c_path =
      src_path.parent_path() / (src_path.stem().string() + ".c");

  if (opts.emit_stage == "c") {
    // --emit=c streams the C source to stdout for inspection / piping.
    std::cout << emitted.code;
    return 0;
  }

  {
    // Scope the ofstream so the file is flushed and closed before cc runs.
    std::ofstream cf(c_path);
    if (!cf) {
      std::cerr << kProgramName << ": error: cannot write '" << c_path.string()
                << "'\n";
      return 1;
    }
    cf << emitted.code;
  }
  if (opts.verbose) {
    std::cout << "  wrote " << c_path.string() << " (" << emitted.code.size()
              << " bytes)\n";
  }

  if (!opts.run_cc) {
    std::cout << kProgramName << ": ok (C emitted, --no-cc)\n";
    return 0;
  }

  // === Invoke system C compiler ===
  // We hand over to cc with -O2 and -std=c17. Anything fancier (link
  // flags, sysroot, sanitisers) belongs on the caretc wrapper script,
  // not hard-coded here.
  // TODO(0.2): replace std::system with fork/exec so we can quote each
  // argv slot safely; today the paths are user-controlled and end up in
  // a shell line.
  std::string cmd =
      opts.cc + " -O2 -std=c17 -o " + opts.output_file + " " + c_path.string();
  if (opts.verbose)
    std::cout << "  $ " << cmd << "\n";
  int rc = std::system(cmd.c_str());
  if (rc != 0) {
    std::cerr << kProgramName << ": error: system C compiler failed (exit "
              << rc << ")\n";
    return 1;
  }
  if (!opts.keep_c) {
    // Best-effort cleanup. Failing to remove the temp .c is not fatal.
    std::error_code rmec;
    std::filesystem::remove(c_path, rmec);
  }
  std::cout << kProgramName << ": ok -> '" << opts.output_file << "'\n";
  return 0;
}
} // namespace

int main(int argc, char **argv) {
  Options opts;
  if (!parse_arguments(argc, argv, opts)) {
    return 1;
  }
  // --help and --version short-circuit before any IO so they always work,
  // even in a directory we cannot read.
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
