# Makefile for building Artisan chess engine with Ninja

# Configuration variables
CMAKE_PATH ?= cmake
NINJA_PATH ?= ninja
BUILD_DIR ?= out/build
INSTALL_DIR ?= out/install

# Detect OS
ifeq ($(OS),Windows_NT)
	DEFAULT_PRESET ?= x64-release
	RM = rmdir /s /q
else
	DEFAULT_PRESET ?= x64-release-linux
	RM = rm -rf
endif

# Use the provided preset or fall back to the default
PRESET ?= $(DEFAULT_PRESET)

# Targets
.PHONY: all clean configure build install rebuild release debug help

all: build

# Configure project using CMake
configure:
	@echo "Configuring project with CMake using preset: $(PRESET)..."
	@${CMAKE_PATH} --preset $(PRESET)

# Build the project
build: configure
	@echo "Building project with Ninja..."
	@${CMAKE_PATH} --build --preset $(PRESET)

# Clean build directory
clean:
	@echo "Cleaning build directory..."
	@if exist ${BUILD_DIR}/$(PRESET) ${RM} ${BUILD_DIR}/$(PRESET)

# Install the build
install: build
	@echo "Installing project..."
	@${CMAKE_PATH} --install ${BUILD_DIR}/$(PRESET)

# Rebuild from scratch
rebuild: clean build

# Build in release mode (now the default)
release:
	@$(MAKE) build PRESET=$(if $(filter-out Windows_NT,$(OS)),x64-release-linux,x64-release)

# Build in debug mode
debug:
	@$(MAKE) build PRESET=$(if $(filter-out Windows_NT,$(OS)),x64-debug-linux,x64-debug)

# Show available commands
help:
	@echo "Artisan Chess Engine Build System"
	@echo "=================================="
	@echo "Available commands:"
	@echo "  make              - Configure and build the project in release mode (default)"
	@echo "  make configure    - Configure project with CMake"
	@echo "  make build        - Build project using Ninja"
	@echo "  make clean        - Remove build directory"
	@echo "  make install      - Install the built project"
	@echo "  make rebuild      - Rebuild from scratch (clean + build)"
	@echo "  make release      - Build in release mode (same as default)"
	@echo "  make debug        - Build in debug mode"
	@echo ""
	@echo "Environment variables:"
	@echo "  CMAKE_PATH        - Path to cmake executable (default: 'cmake')"
	@echo "  NINJA_PATH        - Path to ninja executable (default: 'ninja')"
	@echo "  BUILD_DIR         - Build directory (default: 'out/build')"
	@echo "  PRESET            - CMake preset to use (default: $(DEFAULT_PRESET))"