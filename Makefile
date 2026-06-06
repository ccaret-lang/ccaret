# caretc / C^ — driver Makefile
#
# Convenience targets so you don't have to remember the exact incantation for
# every tool. All targets are intentionally tiny so you can read this file
# and know exactly what each one does.
#
# Quick start:
#     make              # configure + build (Debug)
#     make run FILE=examples/functions.cca
#     make emit FILE=examples/functions.cca
#     make tidy         # clang-tidy-20 on every .cpp
#     make tidy-fix     # clang-tidy-20 --fix (auto-fix what's safe)
#     make format       # clang-format-20 in place
#     make clean
#
# Variables you can override on the command line:
#     BUILD=Release       make build BUILD=Release
#     TIDY=ON             make build TIDY=ON       (enables clang-tidy)
#     FILE=path.cca       make run FILE=path.cca   (default: examples/functions.cca)
#     OUT=foo             make run FILE=x.cca OUT=foo

# ----- toolchain ------------------------------------------------------------
CXX         ?= c++
CC          ?= cc
CARETC      ?= ./bin/ccaret
BUILD_DIR   ?= build
BUILD       ?= Debug
TIDY        ?= OFF
FILE        ?= examples/functions.cca
OUT         ?= a.out
KEEP_C      ?= 0
VERBOSE     ?= 0

# ----- phony ----------------------------------------------------------------
.PHONY: all build rebuild run emit tidy tidy-all tidy-fix format format-check \
        clean mrproper help test config

# ----- help -----------------------------------------------------------------
help:
	@echo "caretc / C^ — Makefile"
	@echo ""
	@echo "Build:"
	@echo "  make / make build     Configure (Debug) and build."
	@echo "  make rebuild          Re-run cmake (in case CMakeLists changed) and build."
	@echo "  make build TIDY=ON    Build with clang-tidy-20 enabled."
	@echo "  make build BUILD=Release"
	@echo ""
	@echo "Run a .cca file:"
	@echo "  make run FILE=examples/functions.cca     compile + execute"
	@echo "  make emit FILE=examples/functions.cca    emit C only (stdout)"
	@echo "  make run FILE=x.cca OUT=myprog          custom output binary"
	@echo "  make run FILE=x.cca KEEP_C=1             keep the generated .c"
	@echo "  make run FILE=x.cca VERBOSE=1           verbose pipeline"
	@echo ""
	@echo "Quality:"
	@echo "  make tidy             clang-tidy-20 on the live pipeline files"
	@echo "  make tidy-all         clang-tidy-20 on every src/*.cpp (incl. stubs)"
	@echo "  make tidy-fix         clang-tidy-20 --fix on the live pipeline files"
	@echo "  make format           clang-format-20 in place"
	@echo "  make format-check     clang-format-20 --dry-run"
	@echo ""
	@echo "Cleanup:"
	@echo "  make clean            remove $(BUILD_DIR)/ and bin/ccaret"
	@echo "  make mrproper         clean + remove generated *.c"
	@echo ""
	@echo "Other:"
	@echo "  make test             run unit tests"
	@echo "  make config           re-run cmake with current flags"

# ----- core build -----------------------------------------------------------
all: build

build:
	@cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD) -DCCARET_CLANG_TIDY=$(TIDY)
	@cmake --build $(BUILD_DIR) --target ccaret -j

rebuild:
	@cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD) -DCCARET_CLANG_TIDY=$(TIDY)
	@cmake --build $(BUILD_DIR) --target ccaret -j

config:
	@cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD) -DCCARET_CLANG_TIDY=$(TIDY)

# ----- run a .cca file ------------------------------------------------------
run: $(CARETC)
	@if [ ! -f "$(FILE)" ]; then \
		echo "error: input file '$(FILE)' not found" >&2 ; exit 1 ; \
	fi
	@FLAGS=""; \
	if [ "$(VERBOSE)" = "1" ]; then FLAGS="$$FLAGS -v"; fi; \
	if [ "$(KEEP_C)" = "1" ]; then FLAGS="$$FLAGS --keep-c"; fi; \
	$(CARETC) -o $(OUT) $$FLAGS $(FILE)
	@echo "----- running ./$(OUT) -----"
	@./$(OUT)

emit: $(CARETC)
	@if [ ! -f "$(FILE)" ]; then \
		echo "error: input file '$(FILE)' not found" >&2 ; exit 1 ; \
	fi
	@$(CARETC) --emit=c $(FILE)

# ----- clang-tidy -----------------------------------------------------------
# Scope clang-tidy to the *live* pipeline files only. The Backend/, IR/, and
# Semantics/ stubs are not in the build and have pre-existing API drift, so
# we skip them. Use `make tidy-all` if you really want every .cpp.
TIDY_SRCS := \
    src/main.cpp \
    src/AST/ast.cpp \
    src/Frontend/Token/token.cpp \
    src/Frontend/Lexer/lexer.cpp \
    src/Frontend/Parser/parser.cpp \
    src/Diagnostics/diagnostics.cpp \
    src/C/c_backend.cpp

TIDY_SRCS_ALL := $(shell find src -name '*.cpp' -type f)

tidy:
	@if [ ! -f "$(BUILD_DIR)/compile_commands.json" ]; then \
		echo "compile_commands.json missing — run 'make build' first." >&2 ; exit 1 ; \
	fi
	@clang-tidy-20 -p $(BUILD_DIR) --use-color $(TIDY_SRCS)

tidy-all:
	@if [ ! -f "$(BUILD_DIR)/compile_commands.json" ]; then \
		echo "compile_commands.json missing — run 'make build' first." >&2 ; exit 1 ; \
	fi
	@clang-tidy-20 -p $(BUILD_DIR) --use-color $(TIDY_SRCS_ALL) || true

tidy-fix:
	@if [ ! -f "$(BUILD_DIR)/compile_commands.json" ]; then \
		echo "compile_commands.json missing — run 'make build' first." >&2 ; exit 1 ; \
	fi
	@clang-tidy-20 -p $(BUILD_DIR) --use-color --fix $(TIDY_SRCS)

# ----- clang-format ---------------------------------------------------------
format:
	@clang-format-20 -i $(TIDY_SRCS)
	@echo "formatted: $(TIDY_SRCS)"

format-check:
	@clang-format-20 --dry-run --Werror $(TIDY_SRCS) && \
		echo "all files are clang-format clean."

# ----- tests ----------------------------------------------------------------
test: build
	@cmake --build $(BUILD_DIR) --target ccaret_tests
	@cd $(BUILD_DIR) && ctest --output-on-failure

# ----- cleanup --------------------------------------------------------------
clean:
	@rm -rf $(BUILD_DIR)
	@rm -f $(CARETC)
	@echo "removed: $(BUILD_DIR)/  and  $(CARETC)"

mrproper: clean
	@find examples -name '*.c' -delete
	@echo "removed generated *.c files"

# ----- convenience: just build the binary path -----------------------------
$(CARETC):
	@$(MAKE) --no-print-directory build
