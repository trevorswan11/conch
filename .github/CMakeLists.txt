cmake_minimum_required(VERSION 3.20)

# Configuration
project(conch_cov)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32 OR NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(ERROR "Coverage reports only available on *nix systems with clang\n")
endif()

find_program(LLVM_PROFDATA llvm-profdata)
find_program(LLVM_COV llvm-cov)

if (NOT LLVM_PROFDATA OR NOT LLVM_COV)
    message(ERROR "Failed to locate llvm-profdata and llvm-cov binaries.\n")
endif()

set(COV_DIR ${CMAKE_BINARY_DIR}/coverage)
set(COV_FLAGS 
    "-g"
    "-DCOV"
    "-DDEBUG"
    "-Wall"
    "-Wextra"
    "-Werror"
    "-Wpedantic"
    "-fprofile-instr-generate"
    "-fcoverage-mapping"
)

set(PROJECT_ROOT "${CMAKE_SOURCE_DIR}/..")
set(WARNING_IGNORE "-w")

# Source file collection
set(SRC_DIR "${PROJECT_ROOT}/src")
file(GLOB_RECURSE ALL_SOURCES "${SRC_DIR}/*.cpp")
set(INCLUDE_DIR "${PROJECT_ROOT}/include")
file(GLOB_RECURSE HEADERS "${INCLUDE_DIR}/*.hpp")

set(LIB_SOURCES ${ALL_SOURCES})
list(FILTER LIB_SOURCES EXCLUDE REGEX "main\\.cpp$")

set(TEST_DIR "${PROJECT_ROOT}/tests")
file(GLOB_RECURSE TEST_SOURCES "${TEST_DIR}/*.cpp")
list(FILTER TEST_SOURCES EXCLUDE REGEX "test_framework/*")
list(FILTER TEST_SOURCES EXCLUDE REGEX "runner\\.cpp")

# Target generation
add_library(conch_core STATIC ${LIB_SOURCES})
target_include_directories(conch_core PUBLIC ${PROJECT_ROOT}/include)
target_compile_options(conch_core PRIVATE ${COV_FLAGS})
target_link_options(conch_core PRIVATE ${COV_FLAGS})

add_library(catch2 STATIC ${PROJECT_ROOT}/tests/test_framework/catch_amalgamated.cpp)
target_include_directories(catch2 PUBLIC ${PROJECT_ROOT}/tests/test_framework)
target_compile_options(catch2 PRIVATE ${WARNING_IGNORE})

add_executable(conch_tests ${TEST_SOURCES})
target_link_libraries(conch_tests PRIVATE catch2 conch_core)
target_include_directories(conch_tests PRIVATE ${PROJECT_ROOT}/include ${PROJECT_ROOT}/tests/helpers)
target_compile_options(conch_tests PRIVATE ${WARNING_IGNORE})

target_compile_options(conch_tests PRIVATE ${COV_FLAGS})
target_link_options(conch_tests PRIVATE ${COV_FLAGS})

add_custom_target(coverage
    COMMAND LLVM_PROFILE_FILE=${COV_DIR}/default.profraw
        $<TARGET_FILE:conch_tests>
    DEPENDS conch_tests
    USES_TERMINAL
)

add_custom_target(coverage-report
    COMMAND ${CMAKE_SOURCE_DIR}/coverage_report.sh
        ${LLVM_PROFDATA}
        ${LLVM_COV}
        $<TARGET_FILE:conch_tests>
        ${COV_DIR}
        ${PROJECT_ROOT}
    DEPENDS coverage
)

add_custom_target(coverage-badge
    COMMAND ${CMAKE_SOURCE_DIR}/coverage_badge.sh
        ${LLVM_COV}
        $<TARGET_FILE:conch_tests>
        ${COV_DIR}
        ${PROJECT_ROOT}
    DEPENDS coverage-report
)

add_custom_target(run-all ALL
    DEPENDS coverage-badge
)
