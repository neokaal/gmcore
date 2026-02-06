# Makefile: helper targets to orchestrate CMake builds using CMakePresets.json
# Use: `make configure`, `make build`, `make run`, `make clean`, `make distclean`.

PRESET ?= default
BUILD_DIR ?= build
CMAKE ?= cmake
JOBS ?= $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
BINARY := $(BUILD_DIR)/gfxlc

.PHONY: all configure build install run run-example clean distclean help

all: build

configure:
	@echo "Configuring with preset $(PRESET)..."
	@$(CMAKE) --preset $(PRESET)

build: configure
	@echo "Building (jobs=$(JOBS))..."
	@$(CMAKE) --build $(BUILD_DIR) --preset $(PRESET) -- -j $(JOBS)

rebuild:
	@echo "Building (jobs=$(JOBS))..."
	@$(CMAKE) --build $(BUILD_DIR) --preset $(PRESET) -- -j $(JOBS)

install: build
	@echo "Installing to $(BUILD_DIR)/install..."
	@$(CMAKE) --install $(BUILD_DIR) --prefix $(BUILD_DIR)/install

run: build
	@echo "Running $(BINARY)"
	@$(BINARY)

run-example: build
	@echo "Running with example lua program..."
	@$(BINARY) examples/01_hello.lua

clean:
	@echo "Cleaning generated CMake files in $(BUILD_DIR)..."
	@rm -rf $(BUILD_DIR)/CMakeFiles $(BUILD_DIR)/CMakeCache.txt $(BUILD_DIR)/cmake_install.cmake $(BUILD_DIR)/compile_commands.json $(BUILD_DIR)/Makefile || true

# distclean removes everything including the build directory
distclean: clean
	@echo "Removing entire build directory $(BUILD_DIR)..."
	@rm -rf $(BUILD_DIR)

help:
	@echo "Makefile targets:"
	@echo "  make configure   - configure project (uses CMakePresets.json preset $(PRESET))"
	@echo "  make build       - configure + build" 
	@echo "  make install     - install to $(BUILD_DIR)/install" 
	@echo "  make run         - build and run binary" 
	@echo "  make run-example - run with example shader (examples/gradient.frag)" 
	@echo "  make clean       - remove generated CMake files" 
	@echo "  make distclean   - remove entire $(BUILD_DIR)" 
