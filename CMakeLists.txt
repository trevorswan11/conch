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

if(MSVC)
    # Completely wipe MSVC builtin flags
    set(CMAKE_C_FLAGS_DEBUG "")
    set(CMAKE_CXX_FLAGS_DEBUG "")

    set(CMAKE_C_FLAGS_RELEASE "")
    set(CMAKE_CXX_FLAGS_RELEASE "")

    set(CMAKE_C_FLAGS_RELWITHDEBINFO "")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "")

    set(CMAKE_C_FLAGS_MINSIZEREL "")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "")

    string(REPLACE "/RTC1" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    string(REPLACE "/RTCsu" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    string(REPLACE "/RTCsu" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

    # MSVC has configurations, but dist needs to be manually populated
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Dist" CACHE STRING "" FORCE)
    set(CMAKE_C_FLAGS_DIST          "")
    set(CMAKE_CXX_FLAGS_DIST        "")
    set(CMAKE_EXE_LINKER_FLAGS_DIST "")
    set(CMAKE_SHARED_LINKER_FLAGS_DIST "")
    set(CMAKE_MODULE_LINKER_FLAGS_DIST "")

    # Set common flags for unix-faithful building
    set(BASE_FLAGS "/W4" "/WX" "/wd4232")
    set(WARNING_IGNORE "/W0")

    set(DIST_FLAGS "/O2" "/DNDEBUG" "/DDIST")
    set(RELEASE_FLAGS "/O1" "/DRELEASE")
    set(DEBUG_FLAGS "/Zi" "/DDEBUG" "/RTC1")
else()
    set(BASE_FLAGS "-Wall" "-Wextra" "-Werror" "-Wpedantic")
    set(WARNING_IGNORE "-w")

    set(DIST_FLAGS "-O3" "-DNDEBUG" "-DDIST")
    set(RELEASE_FLAGS "-O2" "-DRELEASE")
    set(DEBUG_FLAGS "-O0" "-g" "-DDEBUG")
endif()

# Source file collection
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
file(GLOB_RECURSE ALL_SOURCES "${SRC_DIR}/*.c")
set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")
file(GLOB_RECURSE HEADERS "${INCLUDE_DIR}/*.h")

set(LIB_SOURCES ${ALL_SOURCES})
list(FILTER LIB_SOURCES EXCLUDE REGEX "main\\.c$")

set(TEST_DIR "${CMAKE_SOURCE_DIR}/tests")
file(GLOB_RECURSE TEST_SOURCES "${TEST_DIR}/*.cpp")
set(FRAMEWORK_REGEX "test_framework/*")
list(FILTER TEST_SOURCES EXCLUDE REGEX ${FRAMEWORK_REGEX})

function(create_conch_suite TARGET_NAME EXTRA_FLAGS OUT_DIR)
    add_library(conch_core_${TARGET_NAME} STATIC ${LIB_SOURCES})
    target_include_directories(conch_core_${TARGET_NAME} PUBLIC include)
    target_compile_options(conch_core_${TARGET_NAME} PRIVATE ${BASE_FLAGS})

    target_compile_options(conch_core_${TARGET_NAME} PRIVATE ${EXTRA_FLAGS})
    if(NOT MSVC)
        target_link_options(conch_core_${TARGET_NAME} PRIVATE ${EXTRA_FLAGS})
    endif()
    set_target_properties(conch_core_${TARGET_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${OUT_DIR})

    # Fresh catch2 configuration per target
    add_library(catch2_${TARGET_NAME} STATIC tests/test_framework/catch_amalgamated.cpp)
    target_include_directories(catch2_${TARGET_NAME} PUBLIC tests/test_framework)
    target_compile_options(catch2_${TARGET_NAME} PRIVATE ${WARNING_IGNORE})
    set_target_properties(catch2_${TARGET_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${OUT_DIR})

    add_executable(conch_tests_${TARGET_NAME} ${TEST_SOURCES})
    target_link_libraries(conch_tests_${TARGET_NAME} PRIVATE catch2_${TARGET_NAME} conch_core_${TARGET_NAME})
    target_include_directories(conch_tests_${TARGET_NAME} PRIVATE include tests/helpers)
    target_compile_options(conch_tests_${TARGET_NAME} PRIVATE ${WARNING_IGNORE})

    target_compile_options(conch_tests_${TARGET_NAME} PRIVATE ${EXTRA_FLAGS})
    if(NOT MSVC)
        target_link_options(conch_tests_${TARGET_NAME} PRIVATE ${EXTRA_FLAGS})
    endif()
    set_target_properties(conch_tests_${TARGET_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OUT_DIR})
endfunction()

function(create_standard_target EXTRA_FLAGS TARGET_NAME)
    create_conch_suite("${TARGET_NAME}" "${EXTRA_FLAGS}" "${CMAKE_BINARY_DIR}/${TARGET_NAME}")
    add_executable(conch_${TARGET_NAME} src/main.c)
    target_link_libraries(conch_${TARGET_NAME} PRIVATE conch_core_${TARGET_NAME})
    add_custom_target(${TARGET_NAME} DEPENDS conch_tests_${TARGET_NAME} conch_${TARGET_NAME})

    set_target_properties(conch_${TARGET_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${TARGET_NAME}
        OUTPUT_NAME conch
    )
endfunction()

# Standard building & testing targets
create_standard_target("${DIST_FLAGS}" "dist")
create_standard_target("${RELEASE_FLAGS}" "release")
create_standard_target("${DEBUG_FLAGS}" "debug")

enable_testing()
add_test(NAME run_tests_debug COMMAND conch_tests_debug)
add_test(NAME run_tests_release COMMAND conch_tests_release)
add_test(NAME run_tests_dist COMMAND conch_tests_dist)

# Compiler specific configuration
if(MSVC)
    add_custom_target(conch ALL
        DEPENDS
            $<$<CONFIG:Debug>:conch_debug>
            $<$<CONFIG:Release>:conch_release>
            $<$<CONFIG:Dist>:conch_dist>
    )

    add_custom_target(conch_tests ALL
        DEPENDS
            $<$<CONFIG:Debug>:conch_tests_debug>
            $<$<CONFIG:Release>:conch_tests_release>
            $<$<CONFIG:Dist>:conch_tests_dist>
    )
else()
    add_custom_target(conch DEPENDS dist release debug)

    add_custom_target(qtest
        DEPENDS conch_tests_debug
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -R run_tests_debug
        USES_TERMINAL
    )

    add_custom_target(conch_tests
        DEPENDS conch_tests_debug conch_tests_release conch_tests_dist
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -R run_tests_debug
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -R run_tests_release
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -R run_tests_dist
        USES_TERMINAL
    )
endif()

# Standard tooling disabled on windows due to clang/llvm weirdness
if(NOT WIN32 AND (CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID STREQUAL "GNU"))
    # ASan configuration
    if(APPLE AND CMAKE_C_COMPILER_ID MATCHES "Clang")
        # leaks tool is needed and asan pollutes its memory profiler
        set(ASAN_FLAGS "-fsanitize=undefined" "-fno-sanitize-recover=all" "-g")
        set(ASAN_RUN_CMD leaks --atExit -- $<TARGET_FILE:conch_tests_asan>)
    elseif(APPLE AND CMAKE_C_COMPILER_ID STREQUAL "GNU")
        message(ERROR "The GNU Compiler is not supported on macOS")
    elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        set(ASAN_FLAGS "-fsanitize=address" "-fsanitize=undefined" "-g")
        set(ASAN_RUN_CMD $<TARGET_FILE:conch_tests_asan>)
    elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
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
    if(CMAKE_C_COMPILER_ID MATCHES "Clang")
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
                COMMAND ${CMAKE_SOURCE_DIR}/.github/coverage_report.sh
                    ${LLVM_PROFDATA} ${LLVM_COV} $<TARGET_FILE:conch_tests_cov> ${COV_DIR} ${CMAKE_SOURCE_DIR}
                DEPENDS coverage
            )

            add_custom_target(coverage-badge
                COMMAND ${CMAKE_SOURCE_DIR}/.github/coverage_badge.sh
                    ${LLVM_COV} $<TARGET_FILE:conch_tests_cov> ${COV_DIR} ${CMAKE_SOURCE_DIR}
                DEPENDS coverage-report
            )
        else()
            message(WARNING
                    "Failed to locate llvm-profdata and llvm-cov binaries.\n"
                    "   Targets 'coverage-report' and 'coverage-badge' are not available with this configuration."
            )
        endif()
    endif()
elseif(NOT WIN32)
    message(WARNING
            "Skipped ASan and coverage generation as the active compiler is not recognized.\n"
            "Consider using the Clang toolchain by rebuilding with:\n"
            "    CC=clang CXX=clang++ cmake ..")
endif()

# Tooling
file(GLOB_RECURSE TOOLING_SOURCES ${SRC_DIR}/*.c ${INCLUDE_DIR}/*.h ${TEST_DIR}/*.cpp ${TEST_DIR}/*.hpp)
list(FILTER TOOLING_SOURCES EXCLUDE REGEX ${FRAMEWORK_REGEX})

# Lines of code
find_program(CLOC cloc)
if (CLOC)
    add_custom_target(cloc COMMAND ${CLOC} ${SRC_DIR} ${INCLUDE_DIR})
    add_custom_target(cloc-all COMMAND ${CLOC} ${TOOLING_SOURCES})
endif()

# Formatting
find_program(CLANG_FORMAT clang-format)
if(CLANG_FORMAT)
    add_custom_target(fmt COMMAND ${CLANG_FORMAT} -i ${TOOLING_SOURCES})
    add_custom_target(fmt-check COMMAND ${CLANG_FORMAT} --dry-run --Werror ${TOOLING_SOURCES})
endif()
