# Makefile for Nimonspoli CLI + Qt GUI

# Compiler settings
CXX      := g++
CXXFLAGS := -Wall -Wextra -std=c++17 -I include
DEPFLAGS := -MMD -MP

# Directories
SRC_DIR     := src
OBJ_DIR     := build
BIN_DIR     := bin
INCLUDE_DIR := include
DATA_DIR    := data
CONFIG_DIR  := config

# Target executable
ifeq ($(OS),Windows_NT)
	EXE_EXT := .exe
	MKDIR_P := cmake -E make_directory
	RM_RF := cmake -E rm -rf
	PLATFORM := windows
else
	EXE_EXT :=
	MKDIR_P := mkdir -p
	RM_RF := rm -rf
	PLATFORM := linux
endif

PLATFORM_OBJ_DIR := $(OBJ_DIR)/$(PLATFORM)
PLATFORM_BIN_DIR := $(BIN_DIR)/$(PLATFORM)
TARGET := $(PLATFORM_BIN_DIR)/game$(EXE_EXT)
QT_UI_TARGET := nimonspoli_qt_ui
GUI_BUILD_DIR := gui/build/$(PLATFORM)
GUI_TARGET := $(GUI_BUILD_DIR)/$(QT_UI_TARGET)$(EXE_EXT)

# Recursive source finding without relying on Unix find.
rwildcard = $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d))
SRCS := $(call rwildcard,$(SRC_DIR)/,*.cpp)

# Dynamic object mapping: src/xxx/yyy.cpp -> build/<platform>/xxx/yyy.o
OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(PLATFORM_OBJ_DIR)/%.o, $(SRCS))
DEPS := $(OBJS:.o=.d)

# Main targets
all: cli gui-build

cli: directories $(TARGET)

# Create necessary root directories
directories:
	@$(MKDIR_P) $(PLATFORM_OBJ_DIR) $(PLATFORM_BIN_DIR) $(DATA_DIR) $(CONFIG_DIR)

# Link object files to create executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@
	@echo "Build successful! Executable is at $(TARGET)"

# Compile source files into object files
$(PLATFORM_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@$(MKDIR_P) $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

# Run the CLI game
run: cli
	$(TARGET)

# Configure, build, and run the Qt GUI.
ifeq ($(OS),Windows_NT)
check-cmake:
	@where cmake >NUL 2>NUL || (echo CMake tidak ditemukan. Install CMake atau pastikan cmake ada di PATH. && exit /b 1)
else
check-cmake:
	@command -v cmake >/dev/null 2>&1 || { echo "CMake tidak ditemukan di environment Linux/WSL ini."; echo "Jalankan: sudo apt update && sudo apt install -y build-essential make cmake qt6-base-dev qt6-svg-dev"; exit 1; }
endif

gui-configure: check-cmake
	cmake -S gui -B $(GUI_BUILD_DIR)

gui-build: gui-configure
	cmake --build $(GUI_BUILD_DIR) --target $(QT_UI_TARGET) --parallel

gui-run: gui-build
	$(GUI_TARGET)

BuildRun: all
	$(GUI_TARGET)

# Clean up generated files
clean:
	$(RM_RF) $(PLATFORM_OBJ_DIR) $(PLATFORM_BIN_DIR)
	@echo "Cleaned up $(PLATFORM_OBJ_DIR) and $(PLATFORM_BIN_DIR)"

clean-legacy:
	$(RM_RF) $(OBJ_DIR)/core $(OBJ_DIR)/models $(OBJ_DIR)/utils $(OBJ_DIR)/views $(OBJ_DIR)/main.o $(BIN_DIR)/game $(BIN_DIR)/game.exe
	@echo "Cleaned up legacy mixed-platform build outputs"

clean-gui: check-cmake
	$(RM_RF) $(GUI_BUILD_DIR)
	@echo "Cleaned up $(GUI_BUILD_DIR)"

clean-all: clean clean-gui

# Rebuild everything from scratch
rebuild: clean all

rebuild-gui: clean-gui gui-build

help:
	@echo "make all          - build CLI and GUI"
	@echo "make run          - build and run CLI"
	@echo "make BuildRun     - build CLI + GUI, then run GUI"
	@echo "make gui-build    - build GUI only"
	@echo "make gui-run      - build and run GUI only"
	@echo "make clean-all    - remove CLI and GUI build outputs"
	@echo "make clean-legacy - remove old mixed-platform build outputs"

.PHONY: all cli clean clean-legacy clean-gui clean-all rebuild rebuild-gui run check-cmake gui-configure gui-build gui-run BuildRun directories help

-include $(DEPS)
