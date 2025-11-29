cmake_minimum_required(VERSION 3.20)

# Global configuration
project(conch 
    VERSION 0.1.0 
    LANGUAGES C CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -Werror -Wpedantic)

# Source file collection
file(GLOB_RECURSE ALL_SOURCES "src/*.c")
file(GLOB_RECURSE HEADERS "include/*.h")

set(LIB_SOURCES ${ALL_SOURCES})
list(FILTER LIB_SOURCES EXCLUDE REGEX ".*main\\.c$")

file(GLOB_RECURSE TEST_SOURCES "tests/*.cpp")
list(FILTER TEST_SOURCES EXCLUDE REGEX "catch_amalgamated.cpp")

function(create_conch_suite TARGET_NAME EXTRA_FLAGS OUT_DIR)
    add_library(conch_core_${TARGET_NAME} STATIC ${LIB_SOURCES})
    target_include_directories(conch_core_${TARGET_NAME} PUBLIC include)

    if(EXTRA_FLAGS)
        target_compile_options(conch_core_${TARGET_NAME} PRIVATE ${EXTRA_FLAGS})
        target_link_options(conch_core_${TARGET_NAME} PRIVATE ${EXTRA_FLAGS})
    endif()

    set_target_properties(conch_core_${TARGET_NAME} PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY ${OUT_DIR}
    )

    # Fresh catch2 configuration per target
    add_library(catch2_${TARGET_NAME} STATIC tests/test_framework/catch_amalgamated.cpp)
    target_include_directories(catch2_${TARGET_NAME} PUBLIC tests/test_framework)
    if(EXTRA_FLAGS)
        target_compile_options(catch2_${TARGET_NAME} PRIVATE ${EXTRA_FLAGS})
    endif()
    set_target_properties(catch2_${TARGET_NAME} PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY ${OUT_DIR}
    )

    add_executable(conch_tests_${TARGET_NAME} ${TEST_SOURCES})
    target_link_libraries(conch_tests_${TARGET_NAME} PRIVATE catch2_${TARGET_NAME} conch_core_${TARGET_NAME})
    target_include_directories(conch_tests_${TARGET_NAME} PRIVATE include tests/helpers)

    set_target_properties(conch_tests_${TARGET_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${OUT_DIR}
    )

    if(EXTRA_FLAGS)
        target_compile_options(conch_tests_${TARGET_NAME} PRIVATE ${EXTRA_FLAGS})
        target_link_options(conch_tests_${TARGET_NAME} PRIVATE ${EXTRA_FLAGS})
    endif()
endfunction()

function(create_standard_target EXTRA_FLAGS TARGET_NAME)
    create_conch_suite("${TARGET_NAME}" "${EXTRA_FLAGS}" "${CMAKE_BINARY_DIR}/${TARGET_NAME}")
    add_executable(conch_${TARGET_NAME} src/main.c)
    target_link_libraries(conch_${TARGET_NAME} PRIVATE conch_core_${TARGET_NAME})
    add_custom_target(${TARGET_NAME}
        DEPENDS conch_tests_${TARGET_NAME} conch_${TARGET_NAME}
    )

    set_target_properties(conch_${TARGET_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${TARGET_NAME}
        OUTPUT_NAME conch
    )
endfunction()


# Standard build targets
set(DIST_FLAGS "-O3" "-DNDEBUG" "-DDIST")
create_standard_target("${DIST_FLAGS}" "dist")

set(RELEASE_FLAGS "-O2" "-DRELEASE")
create_standard_target("${RELEASE_FLAGS}" "release")

set(DEBUG_FLAGS "-O0" "-g" "-DDEBUG")
create_standard_target("${DEBUG_FLAGS}" "debug")

enable_testing()
add_test(NAME run_tests_debug COMMAND conch_tests_debug)
add_test(NAME run_tests_release COMMAND conch_tests_release)
add_test(NAME run_tests_dist COMMAND conch_tests_dist)

# Standard tooling disabled on windows due to clang/llvm weirdness
if(NOT WIN32)
    if (NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
        message(WARNING
            "ASan and Coverage target will be generated, but compiler is not clang.\n"
            "For correct builds, use the clang/clang++ toolchain"
        )
    endif()

    # ASan configuration - address doesn't work on macOS by default
    if(APPLE)
        set(ASAN_FLAGS "-fsanitize=undefined" "-fno-sanitize-recover=all" "-g")
        set(ASAN_RUN_CMD leaks --atExit -- $<TARGET_FILE:conch_tests_asan>)
    else()
        set(ASAN_FLAGS "-fsanitize=address,undefined" "-fno-sanitize-recover=all" "-g")
        set(ASAN_RUN_CMD $<TARGET_FILE:conch_tests_asan>)
    endif()

    create_conch_suite("asan" "${ASAN_FLAGS}" "${CMAKE_BINARY_DIR}/asan")
    add_custom_target(asan
        COMMAND ${ASAN_RUN_CMD}
        DEPENDS conch_tests_asan
        USES_TERMINAL
    )

    # Coverage configurations
    set(COV_FLAGS "-fprofile-instr-generate" "-fcoverage-mapping" "-g")
    set(COV_DIR ${CMAKE_BINARY_DIR}/cov)
    create_conch_suite("cov" "${COV_FLAGS}" "${COV_DIR}")
    add_custom_target(coverage
        COMMAND LLVM_PROFILE_FILE=${COV_DIR}/default.profraw $<TARGET_FILE:conch_tests_cov>
        DEPENDS conch_tests_cov
        USES_TERMINAL
    )

    find_program(LLVM_PROFDATA llvm-profdata)
    find_program(LLVM_COV llvm-cov)

    if(LLVM_PROFDATA AND LLVM_COV)
        add_custom_target(coverage-report
            COMMAND ${LLVM_PROFDATA} merge -sparse ${COV_DIR}/default.profraw -o ${COV_DIR}/default.profdata
            COMMAND ${LLVM_COV} show $<TARGET_FILE:conch_tests_cov> 
                -instr-profile=${COV_DIR}/default.profdata 
                -format=html 
                -output-dir=${COV_DIR}/coverage_report 
                -ignore-filename-regex='${CMAKE_SOURCE_DIR}/tests/.*'
            DEPENDS coverage
        )

        add_custom_target(coverage-badge
            COMMAND ${LLVM_COV} report $<TARGET_FILE:conch_tests_cov>
                -instr-profile=${COV_DIR}/default.profdata 
                -ignore-filename-regex='${CMAKE_SOURCE_DIR}/tests/.*' > ${COV_DIR}/coverage_summary.txt
            COMMAND ${CMAKE_SOURCE_DIR}/.github/coverage_badge.sh ${COV_DIR}/coverage_summary.txt ${COV_DIR}/coverage.svg
            DEPENDS coverage-report
        )
    endif()
endif()

# Formatting
find_program(CLANG_FORMAT clang-format)
if(CLANG_FORMAT)
    file(GLOB_RECURSE FMT_SOURCES src/*.c include/*.h tests/*.cpp tests/*.hpp)
    list(FILTER FMT_SOURCES EXCLUDE REGEX "catch_amalgamated.cpp")
    list(FILTER FMT_SOURCES EXCLUDE REGEX "catch_amalgamated.hpp")

    add_custom_target(fmt
        COMMAND ${CLANG_FORMAT} -i ${FMT_SOURCES}
    )

    add_custom_target(fmt-check
        COMMAND ${CLANG_FORMAT} --dry-run --Werror ${FMT_SOURCES}
    )
endif()
