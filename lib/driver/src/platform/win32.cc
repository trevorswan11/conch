#include <config.h>

// "Abandon hope, all ye who enter here"
#if PLATFORM_WINDOWS
#    include <atomic>
#    include <windows.h>

#    include "platform/win32.hh"
#    include "types.hh"

namespace ghoti::win32 {

namespace {

std::atomic<i32> REF_COUNT{0};
UINT             ORIGINAL_CODE_PAGE   = 0;
DWORD            ORIGINAL_STDOUT_MODE = 0;
DWORD            ORIGINAL_STDERR_MODE = 0;

} // namespace

RichConsole::RichConsole() noexcept {
    if (REF_COUNT.fetch_add(1) > 0) { return; }
    ORIGINAL_CODE_PAGE = GetConsoleOutputCP();
    SetConsoleOutputCP(CP_UTF8);

    if (auto stdout_h = GetStdHandle(STD_OUTPUT_HANDLE); stdout_h != INVALID_HANDLE_VALUE) {
        GetConsoleMode(stdout_h, &ORIGINAL_STDOUT_MODE);
        SetConsoleMode(stdout_h, ORIGINAL_STDOUT_MODE | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }

    if (auto stderr_h = GetStdHandle(STD_ERROR_HANDLE); stderr_h != INVALID_HANDLE_VALUE) {
        GetConsoleMode(stderr_h, &ORIGINAL_STDERR_MODE);
        SetConsoleMode(stderr_h, ORIGINAL_STDERR_MODE | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
}

RichConsole::~RichConsole() {
    if (REF_COUNT.fetch_sub(1) != 1) { return; }
    SetConsoleOutputCP(ORIGINAL_CODE_PAGE);

    if (auto stdout_h = GetStdHandle(STD_OUTPUT_HANDLE); stdout_h != INVALID_HANDLE_VALUE) {
        SetConsoleMode(stdout_h, ORIGINAL_STDOUT_MODE);
    }

    if (auto stderr_h = GetStdHandle(STD_ERROR_HANDLE); stderr_h != INVALID_HANDLE_VALUE) {
        SetConsoleMode(stderr_h, ORIGINAL_STDERR_MODE);
    }
}

} // namespace ghoti::win32

#endif
