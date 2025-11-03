TARGET := conch
SRC_DIR := src
INC_DIR := include
TEST_DIR := tests
BUILD_DIR := build
BIN_ROOT := bin

CC ?= clang
CXX ?= clang++

DEPFLAGS = -MMD -MP
INCLUDES := -I$(INC_DIR)

rwildcard=$(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d))

SRCS := $(call rwildcard, $(SRC_DIR)/, *.c)
HEADERS := $(wildcard $(INC_DIR)/*.h)

TEST_SRCS := $(wildcard $(TEST_DIR)/*.cpp)
TEST_OBJS := $(patsubst $(TEST_DIR)/%.cpp,$(BUILD_DIR)/tests/%.o,$(TEST_SRCS))
TEST_BIN := $(BIN_ROOT)/tests/run_tests$(EXE)

FMT_SRCS := $(SRCS) \
            $(call rwildcard,$(INC_DIR)/,*.h) \
            $(filter-out $(TEST_DIR)/test_framework/%, $(call rwildcard,$(TEST_DIR)/,*.cpp))

# ================ CROSS PLATFORM SUPPORT ================

ifeq ($(OS),Windows_NT)
    SHELL := cmd.exe
    RM := del /Q
    MKDIR = if not exist "$(subst /,\\,$(1))" mkdir "$(subst /,\\,$(1))"
    EXE := .exe
else
    SHELL := /bin/sh
    RM := rm -f
    MKDIR = mkdir -p $(1)
    EXE :=
endif

# ================ DIST CONFIG ================

OBJ_DIR_DIST := $(BUILD_DIR)/dist
BIN_DIR_DIST := $(BIN_ROOT)/dist
CFLAGS_DIST := -std=c17 -O3 -Wall -Wextra -Werror -Wpedantic $(INCLUDES) $(DEPFLAGS) -DDIST -DNDEBUG

OBJS_DIST := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR_DIST)/%.o,$(SRCS))
TARGET_BIN_DIST := $(BIN_DIR_DIST)/$(TARGET)$(EXE)

# ================ RELEASE CONFIG ================

OBJ_DIR_RELEASE := $(BUILD_DIR)/release
BIN_DIR_RELEASE := $(BIN_ROOT)/release
CFLAGS_RELEASE := -std=c17 -O2 -Wall -Wextra -Werror -Wpedantic $(INCLUDES) $(DEPFLAGS) -DRELEASE

OBJS_RELEASE := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR_RELEASE)/%.o,$(SRCS))
TARGET_BIN_RELEASE := $(BIN_DIR_RELEASE)/$(TARGET)$(EXE)

# ================ DEBUG CONFIG ================

OBJ_DIR_DEBUG := $(BUILD_DIR)/debug
BIN_DIR_DEBUG := $(BIN_ROOT)/debug
CFLAGS_DEBUG := -std=c17 -O0 -Wall -Wextra -Werror -Wpedantic -g $(INCLUDES) $(DEPFLAGS) -DDEBUG

OBJS_DEBUG := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR_DEBUG)/%.o,$(SRCS))
TARGET_BIN_DEBUG := $(BIN_DIR_DEBUG)/$(TARGET)$(EXE)

# ================ BUILD TARGETS ================

default: release
install: dist
all: dist release debug
dist: $(TARGET_BIN_DIST)
release: $(TARGET_BIN_RELEASE)
debug: $(TARGET_BIN_DEBUG)

test: $(TEST_BIN)
	@$(TEST_BIN)

# ================ TESTING BUILD ================

OBJ_DIR_TEST := $(BUILD_DIR)/tests
OBJS_TEST := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR_TEST)/%.o,$(SRCS))
CFLAGS_TEST := -std=c17 -O0 -Wall -Wextra -Werror -Wpedantic $(INCLUDES) $(DEPFLAGS) -DDEBUG -DTEST

TEST_OBJS := $(patsubst $(TEST_DIR)/%.cpp,$(BUILD_DIR)/tests/%.o,$(TEST_SRCS))
CATCH_OBJ := $(BUILD_DIR)/tests/catch_amalgamated.o
LIB_OBJS_FOR_TESTS := $(filter-out $(OBJ_DIR_TEST)/main.o,$(OBJS_TEST))
CXXFLAGS_TEST = -std=c++20 -O2 -Wall -Wextra $(INCLUDES) $(DEPFLAGS) -DTEST

$(OBJ_DIR_TEST)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	@$(call MKDIR,$(dir $@))
	$(CC) $(CFLAGS_TEST) -c $< -o $@

# Compile test source files
$(BUILD_DIR)/tests/%.o: $(TEST_DIR)/%.cpp $(HEADERS)
	@$(call MKDIR,$(dir $@))
	$(CXX) $(CXXFLAGS_TEST) -c $< -o $@

# Compile Catch2 itself
$(CATCH_OBJ): $(TEST_DIR)/test_framework/catch_amalgamated.cpp
	@$(call MKDIR,$(dir $@))
	$(CXX) $(CXXFLAGS_TEST) -c $< -o $@

# Link all test objects into the test binary
$(TEST_BIN): $(CATCH_OBJ) $(TEST_OBJS) $(LIB_OBJS_FOR_TESTS)
	@$(call MKDIR,$(dir $@))
	$(CXX) $(CXXFLAGS_TEST) -o $@ $^

# ================ BINARY DIRECTORIES ================

$(TARGET_BIN_DIST): $(OBJS_DIST)
	@$(call MKDIR,$(BIN_DIR_DIST))
	$(CC) $(CFLAGS_DIST) -o $@ $^

$(TARGET_BIN_RELEASE): $(OBJS_RELEASE)
	@$(call MKDIR,$(BIN_DIR_RELEASE))
	$(CC) $(CFLAGS_RELEASE) -o $@ $^

$(TARGET_BIN_DEBUG): $(OBJS_DEBUG)
	@$(call MKDIR,$(BIN_DIR_DEBUG))
	$(CC) $(CFLAGS_DEBUG) -o $@ $^

# ================ OBJECT DIRECTORIES ================

$(OBJ_DIR_DIST)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	@$(call MKDIR,$(dir $@))
	$(CC) $(CFLAGS_DIST) -c $< -o $@

$(OBJ_DIR_RELEASE)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	@$(call MKDIR,$(dir $@))
	$(CC) $(CFLAGS_RELEASE) -c $< -o $@

$(OBJ_DIR_DEBUG)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	@$(call MKDIR,$(dir $@))
	$(CC) $(CFLAGS_DEBUG) -c $< -o $@

# ================ INCLUDES ================

-include $(OBJS_DIST:.o=.d)
-include $(OBJS_RELEASE:.o=.d)
-include $(OBJS_DEBUG:.o=.d)
-include $(TEST_OBJS:.o=.d)

# ================ OTHER TARGETS ================

ARGS ?=

run: run-release

run-dist: $(TARGET_BIN_DIST)
	@$(TARGET_BIN_DIST) $(ARGS)

run-release: $(TARGET_BIN_RELEASE)
	@$(TARGET_BIN_RELEASE) $(ARGS)

run-debug: $(TARGET_BIN_DEBUG)
	@$(TARGET_BIN_DEBUG) $(ARGS)

clean:
ifeq ($(OS),Windows_NT)
	@if exist "$(BUILD_DIR)" rmdir /S /Q "$(BUILD_DIR)"
	@if exist "$(BIN_ROOT)" rmdir /S /Q "$(BIN_ROOT)"
else
	@rm -rf $(BUILD_DIR)
	@rm -rf $(BIN_ROOT)
endif

# ================ FORMATTING ================

fmt:
	@clang-format -i $(FMT_SRCS)

fmt-check:
	@clang-format --dry-run --Werror $(FMT_SRCS)

# ================ HELP ME ================

help: 
	@printf "\
\n\
To build and run the project, type:\n\
\n\
make [target] [ARGS=]\n\
\n\
Build Specific Targets:\n\
\n\
default           > Builds the release configuration (default)\n\
install           > Builds the dist configuration\n\
all               > Builds all optimization configurations (dist, release, debug)\n\
dist              > Max optimization, assertions disabled\n\
release           > Slightly fewer optimizations, no assertions enabled\n\
debug             > No optimization, assertions enabled, debug symbols included\n\
test              > Build and run the project's tests\n\
run               > Build and run the release binary\n\
run-dist          > Build and run the dist binary\n\
run-release       > Build and run the release binary\n\
run-debug         > Build and run the debug binary\n\
fmt               > Format all source and header files with clang-format\n\
fmt-check         > Check formatting rules without modifying files\n\
clean             > Remove object files, dependency files, and binaries\n\
\n\
General Targets:\n\
\n\
help              > Print this help menu\n\
"

.PHONY: default install all dist release debug \
		run run-dist run-release run-debug \
		test clean fmt fmt-check help
