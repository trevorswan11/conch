TARGET := conch
SRC_DIR := src
INC_DIR := include
TEST_DIR := tests
BUILD_DIR := build
BIN_ROOT := bin

CC ?= clang
CXX ?= clang++

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

# ================ SOURCE CONFIG ================

DEPFLAGS = -MMD -MP
INCLUDES := -I$(INC_DIR)
TEST_INCLUDES := -I$(INC_DIR) -I$(TEST_DIR)/helpers -I$(TEST_DIR)/test_framework

rwildcard = $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d))

SRCS := $(call rwildcard, $(SRC_DIR)/, *.c)
HEADERS := $(wildcard $(INC_DIR)/*.h)

TEST_SRCS := $(filter-out $(TEST_DIR)/test_framework/catch_amalgamated.cpp, \
             $(call rwildcard, $(TEST_DIR)/, *.cpp))
TEST_OBJS := $(patsubst $(TEST_DIR)/%.cpp,$(BUILD_DIR)/tests/%.o,$(TEST_SRCS))
TEST_BIN := $(BIN_ROOT)/tests/run_tests$(EXE)
COVERAGE_BIN := $(BIN_ROOT)/coverage/run_tests$(EXE)
ASAN_BIN := $(BIN_ROOT)/asan/run_tests$(EXE)
LSAN_BIN := $(BIN_ROOT)/lsan/run_tests$(EXE)

FMT_SRCS := $(SRCS) \
            $(call rwildcard,$(INC_DIR)/,*.h) \
            $(filter-out $(TEST_DIR)/test_framework/%, $(call rwildcard,$(TEST_DIR)/,*.cpp))

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

BIN_DIR_COVERAGE := $(BIN_ROOT)/coverage
coverage: CC := clang
coverage: CXX := clang++
ifeq ($(OS),Windows_NT)
coverage: 
	$(error Coverage is not supported on Windows)
else
coverage: $(COVERAGE_BIN)
	@LLVM_PROFILE_FILE=$(BIN_DIR_COVERAGE)/default.profraw $(COVERAGE_BIN)
endif

asan: CC := clang
asan: CXX := clang++
ifeq ($(OS),Windows_NT)
asan: 
	$(error Sanitizers are not supported on Windows)
else
ifeq ($(shell uname -s),Darwin)
asan: $(LSAN_BIN)
	@leaks --atExit -- $(LSAN_BIN)
else
asan: $(ASAN_BIN)
	@$(ASAN_BIN)
endif
endif

# ================ TESTING BUILD ================

OBJ_DIR_TEST := $(BUILD_DIR)/tests
OBJS_TEST := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR_TEST)/%.o,$(SRCS))
CFLAGS_TEST := -std=c17 -O0 -Wall -Wextra -Werror -Wpedantic -g $(INCLUDES) $(DEPFLAGS) -DDEBUG -DTEST

TEST_OBJS := $(patsubst $(TEST_DIR)/%.cpp,$(BUILD_DIR)/tests/%.o,$(TEST_SRCS))
CATCH_OBJ_TESTS := $(BUILD_DIR)/catch/tests/catch_amalgamated.o
LIB_OBJS_FOR_TESTS := $(filter-out $(OBJ_DIR_TEST)/main.o,$(OBJS_TEST))
CXXFLAGS_TEST = -std=c++20 -O0 -Wall -Wextra -g $(TEST_INCLUDES) $(DEPFLAGS) -DTEST

$(OBJ_DIR_TEST)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	@$(call MKDIR,$(dir $@))
	$(CC) $(CFLAGS_TEST) -c $< -o $@

$(BUILD_DIR)/tests/%.o: $(TEST_DIR)/%.cpp $(HEADERS)
	@$(call MKDIR,$(dir $@))
	$(CXX) $(CXXFLAGS_TEST) -c $< -o $@

$(CATCH_OBJ_TESTS): $(TEST_DIR)/test_framework/catch_amalgamated.cpp
	@$(call MKDIR,$(dir $@))
	$(CXX) $(CXXFLAGS_TEST) -c $< -o $@

$(TEST_BIN): $(CATCH_OBJ_TESTS) $(TEST_OBJS) $(LIB_OBJS_FOR_TESTS)
	@$(call MKDIR,$(dir $@))
	$(CXX) $(CXXFLAGS_TEST) -o $@ $^

# ================ ASAN BUILD ================

OBJ_DIR_ASAN := $(BUILD_DIR)/asan
OBJS_ASAN := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR_ASAN)/%.o,$(SRCS))
CFLAGS_ASAN := -std=c17 -O0 -Wall -Wextra -Werror -Wpedantic -g -fsanitize=address,undefined -fno-sanitize-recover=all $(INCLUDES) $(DEPFLAGS) -DDEBUG -DTEST -DASAN

ASAN_TEST_OBJS := $(patsubst $(TEST_DIR)/%.cpp,$(BUILD_DIR)/asan/%.o,$(TEST_SRCS))
CATCH_OBJ_ASAN := $(BUILD_DIR)/catch/asan/catch_amalgamated.o
LIB_OBJS_FOR_ASAN := $(filter-out $(OBJ_DIR_ASAN)/main.o,$(OBJS_ASAN))
CXXFLAGS_ASAN = -std=c++20 -O0 -Wall -Wextra -g -fsanitize=address,undefined -fno-sanitize-recover=all $(TEST_INCLUDES) $(DEPFLAGS) -DTEST -DASAN

$(OBJ_DIR_ASAN)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	@$(call MKDIR,$(dir $@))
	$(CC) $(CFLAGS_ASAN) -c $< -o $@

$(BUILD_DIR)/asan/%.o: $(TEST_DIR)/%.cpp $(HEADERS)
	@$(call MKDIR,$(dir $@))
	$(CXX) $(CXXFLAGS_ASAN) -c $< -o $@

$(CATCH_OBJ_ASAN): $(TEST_DIR)/test_framework/catch_amalgamated.cpp
	@$(call MKDIR,$(dir $@))
	$(CXX) $(CXXFLAGS_ASAN) -c $< -o $@

$(ASAN_BIN): $(CATCH_OBJ_ASAN) $(ASAN_TEST_OBJS) $(LIB_OBJS_FOR_ASAN)
	@$(call MKDIR,$(dir $@))
	$(CXX) $(CXXFLAGS_ASAN) -o $@ $^

# ================ LSAN BUILD ================

OBJ_DIR_LSAN := $(BUILD_DIR)/lsan
OBJS_LSAN := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR_LSAN)/%.o,$(SRCS))
CFLAGS_LSAN := -std=c17 -O0 -Wall -Wextra -Werror -Wpedantic -g -fsanitize=undefined -fno-sanitize-recover=all $(INCLUDES) $(DEPFLAGS) -DDEBUG -DTEST -DLSAN

LSAN_TEST_OBJS := $(patsubst $(TEST_DIR)/%.cpp,$(BUILD_DIR)/lsan/%.o,$(TEST_SRCS))
CATCH_OBJ_LSAN := $(BUILD_DIR)/catch/lsan/catch_amalgamated.o
LIB_OBJS_FOR_LSAN := $(filter-out $(OBJ_DIR_LSAN)/main.o,$(OBJS_LSAN))
CXXFLAGS_LSAN = -std=c++20 -O0 -Wall -Wextra -g -fsanitize=undefined -fno-sanitize-recover=all $(TEST_INCLUDES) $(DEPFLAGS) -DTEST -DLSAN

$(OBJ_DIR_LSAN)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	@$(call MKDIR,$(dir $@))
	$(CC) $(CFLAGS_LSAN) -c $< -o $@

$(BUILD_DIR)/lsan/%.o: $(TEST_DIR)/%.cpp $(HEADERS)
	@$(call MKDIR,$(dir $@))
	$(CXX) $(CXXFLAGS_LSAN) -c $< -o $@

$(CATCH_OBJ_LSAN): $(TEST_DIR)/test_framework/catch_amalgamated.cpp
	@$(call MKDIR,$(dir $@))
	$(CXX) $(CXXFLAGS_LSAN) -c $< -o $@

$(LSAN_BIN): $(CATCH_OBJ_LSAN) $(LSAN_TEST_OBJS) $(LIB_OBJS_FOR_LSAN)
	@$(call MKDIR,$(dir $@))
	$(CXX) $(CXXFLAGS_LSAN) -o $@ $^

# ================ COVERAGE BUILD ================

OBJ_DIR_COVERAGE := $(BUILD_DIR)/coverage
OBJS_COVERAGE := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR_COVERAGE)/%.o,$(SRCS))
CFLAGS_COVERAGE := -std=c17 -fprofile-instr-generate -fcoverage-mapping -O0 -g -Wall -Wextra -Werror -Wpedantic $(INCLUDES) $(DEPFLAGS) -DDEBUG -DCOVERAGE

COVERAGE_OBJS := $(patsubst $(TEST_DIR)/%.cpp,$(BUILD_DIR)/coverage/%.o,$(TEST_SRCS))
CATCH_OBJ_COVERAGE := $(BUILD_DIR)/catch/coverage/catch_amalgamated.o
LIB_OBJS_FOR_COVERAGES := $(filter-out $(OBJ_DIR_COVERAGE)/main.o,$(OBJS_COVERAGE))
CXXFLAGS_COVERAGE = -std=c++20 -fprofile-instr-generate -fcoverage-mapping -O0 -g -Wall -Wextra $(TEST_INCLUDES) $(DEPFLAGS) -DCOVERAGE

$(OBJ_DIR_COVERAGE)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	@$(call MKDIR,$(dir $@))
	$(CC) $(CFLAGS_COVERAGE) -c $< -o $@

$(BUILD_DIR)/coverage/%.o: $(TEST_DIR)/%.cpp $(HEADERS)
	@$(call MKDIR,$(dir $@))
	$(CXX) $(CXXFLAGS_COVERAGE) -c $< -o $@

$(CATCH_OBJ_COVERAGE): $(TEST_DIR)/test_framework/catch_amalgamated.cpp
	@$(call MKDIR,$(dir $@))
	$(CXX) $(CXXFLAGS_COVERAGE) -c $< -o $@

$(COVERAGE_BIN): $(CATCH_OBJ_COVERAGE) $(COVERAGE_OBJS) $(LIB_OBJS_FOR_COVERAGES)
	@$(call MKDIR,$(dir $@))
	$(CXX) $(CXXFLAGS_COVERAGE) -o $@ $^

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
-include $(ASAN_TEST_OBJS:.o=.d)
-include $(OBJS_ASAN:.o=.d)

# ================ OTHER TARGETS ================

coverage-report: coverage
	@mkdir -p coverage_report
	@llvm-profdata merge -sparse $(BIN_DIR_COVERAGE)/default.profraw -o $(BIN_DIR_COVERAGE)/default.profdata
	@llvm-cov show $(COVERAGE_BIN) \
		-instr-profile=$(BIN_DIR_COVERAGE)/default.profdata \
		-format=html \
		-output-dir=coverage_report \
		-ignore-filename-regex='.*tests/.*'

coverage-badge: coverage-report
	@llvm-cov report $(COVERAGE_BIN) \
		-instr-profile=$(BIN_DIR_COVERAGE)/default.profdata \
		-ignore-filename-regex='.*tests/.*' > coverage_summary.txt
	@TOTAL_PERCENT=$$(awk '/TOTAL/ {gsub(/%/,""); print int($$10)}' coverage_summary.txt); \
	echo "Total coverage: $$TOTAL_PERCENT%"; \
	curl -o coverage.svg "https://img.shields.io/badge/Coverage-$$TOTAL_PERCENT%25-pink"

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

cltest:
ifeq ($(OS),Windows_NT)
	@if exist "$(BUILD_DIR)/tests" rmdir /S /Q "$(BUILD_DIR)/tests"
	@if exist "$(BIN_ROOT)/tests" rmdir /S /Q "$(BIN_ROOT)/tests"
else
	@rm -rf $(BUILD_DIR)/tests
	@rm -rf $(BIN_ROOT)/tests
endif

clasan:
ifeq ($(OS),Windows_NT)
	@if exist "$(BUILD_DIR)/asan" rmdir /S /Q "$(BUILD_DIR)/asan"
	@if exist "$(BIN_ROOT)/asan" rmdir /S /Q "$(BIN_ROOT)/asan"
else
	@rm -rf $(BUILD_DIR)/asan
	@rm -rf $(BIN_ROOT)/asan
endif

clcov:
ifeq ($(OS),Windows_NT)
	@if exist "$(BUILD_DIR)/coverage" rmdir /S /Q "$(BUILD_DIR)/coverage"
	@if exist "$(BIN_ROOT)/coverage" rmdir /S /Q "$(BIN_ROOT)/coverage"
else
	@rm -rf $(BUILD_DIR)/coverage
	@rm -rf $(BIN_ROOT)/coverage
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
release           > Slightly fewer optimizations, assertions enabled\n\
debug             > No optimization, assertions enabled, debug symbols included\n\
test              > Build and run the project's tests\n\
run               > Build and run the release binary\n\
run-dist          > Build and run the dist binary\n\
run-release       > Build and run the release binary\n\
run-debug         > Build and run the debug binary\n\
coverage          > Compile the tests with coverage instrumentation and record its data\n\
coverage-report   > Gather the test coverage information and emit a comprehensive report\n\
coverage-badge    > Gather the test coverage information and create its GitHub badge\n\
asan              > Build and run the tests with address and UB sanitizers\n\
fmt               > Format all source and header files with clang-format\n\
fmt-check         > Check formatting rules without modifying files\n\
clean             > Remove object files, dependency files, and binaries\n\
cltest            > Remove test object files, dependency files, and binaries, excluding Catch2 objects\n\
clasan            > Remove asan object files, dependency files, and binaries, excluding Catch2 objects\n\
clcov             > Remove coverage object files, dependency files, and binaries, excluding Catch2 objects\n\
\n\
General Targets:\n\
\n\
help              > Print this help menu\n\
"

.PHONY: default install all dist release debug \
		test run run-dist run-release run-debug \
		coverage coverage-report coverage-badge \
		asan fmt fmt-check clean cltest clasan \
		clcov help
