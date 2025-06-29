# Makefile for building Artisan chess engine with Ninja

# Configuration variables
CMAKE_PATH ?= cmake
NINJA_PATH ?= ninja
BUILD_DIR ?= out/build
INSTALL_DIR ?= out/install

# Output executable name (can be overridden with EXE=name)
EXE ?= artisan

# Detect OS
ifeq ($(OS),Windows_NT)
	DEFAULT_PRESET ?= x64-release
	RM = rmdir /s /q
	EXE_EXT = .exe
else
	DEFAULT_PRESET ?= x64-release-linux
	RM = rm -rf
	EXE_EXT =
endif

# Use the provided preset or fall back to the default
PRESET ?= $(DEFAULT_PRESET)

# Full path to the output executable
OUTPUT_BIN = $(CURDIR)/$(EXE)$(EXE_EXT)

# Targets
.PHONY: all clean configure build install rebuild release debug help copy_exe

all: build copy_exe

# Configure project using CMake
configure:
	@echo "Configuring project with CMake using preset: $(PRESET)..."
	@$(CMAKE_PATH) --preset $(PRESET) -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=$(BUILD_DIR)/$(PRESET)

# Build the project
build: configure
	@echo "Building project with Ninja..."
	@cd $(BUILD_DIR)/$(PRESET) && $(NINJA_PATH)

# Copy the executable to the Makefile directory with the specified name
copy_exe:
	@echo "Copying executable to $(OUTPUT_BIN)..."
	@cp $(BUILD_DIR)/$(PRESET)/src/Artisan$(EXE_EXT) $(OUTPUT_BIN)
	@echo "Build complete: $(EXE)$(EXE_EXT)"

# Clean build directory and executable
clean:
	@echo "Cleaning build directory and executable..."
	@[ -d $(BUILD_DIR)/$(PRESET) ] && $(RM) $(BUILD_DIR)/$(PRESET) || true
	@[ -f $(OUTPUT_BIN) ] && rm $(OUTPUT_BIN) || true

# Install the build
install: build
	@echo "Installing project..."
	@$(CMAKE_PATH) --install $(BUILD_DIR)/$(PRESET)

# Rebuild from scratch
rebuild: clean build copy_exe

# Build in release mode (now the default)
release:
	@$(MAKE) build PRESET=$(if $(filter-out Windows_NT,$(OS)),x64-release-linux,x64-release)
	@$(MAKE) copy_exe

# Build in debug mode
debug:
	@$(MAKE) build PRESET=$(if $(filter-out Windows_NT,$(OS)),x64-debug-linux,x64-debug)
	@$(MAKE) copy_exe

# Show available commands
help:
	@echo "Artisan Chess Engine Build System"
	@echo "=================================="
	@echo "Available commands:"
	@echo "  make              - Configure, build the project in release mode, and copy executable (default)"
	@echo "  make configure    - Configure project with CMake"
	@echo "  make build        - Build project using Ninja"
	@echo "  make clean        - Remove build directory and executable"
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
	@echo "  EXE               - Output executable name (default: artisan)"
	@echo ""
	@echo "Examples:"
	@echo "  make              - Build release version as 'artisan'"
	@echo "  make EXE=engine   - Build release version as 'engine'"
	@echo "  make debug EXE=artisan_debug - Build debug version as 'artisan_debug'"