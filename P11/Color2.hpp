#pragma once
#include <windows.h>

/*
    Console color utilities for SERVER
    Windows-only (WinAPI)
*/

namespace ServerColor {

    constexpr WORD DEFAULT = 7;

    constexpr WORD BLACK = 0;
    constexpr WORD BLUE = 1;
    constexpr WORD GREEN = 2;
    constexpr WORD CYAN = 3;
    constexpr WORD RED = 4;
    constexpr WORD MAGENTA = 5;
    constexpr WORD YELLOW = 6;
    constexpr WORD WHITE = 7;

    constexpr WORD BRIGHT_BLUE = 9;
    constexpr WORD BRIGHT_GREEN = 10;
    constexpr WORD BRIGHT_CYAN = 11;
    constexpr WORD BRIGHT_RED = 12;
    constexpr WORD BRIGHT_MAGENTA = 13;
    constexpr WORD BRIGHT_YELLOW = 14;
    constexpr WORD BRIGHT_WHITE = 15;

    inline void set(WORD color) {
        SetConsoleTextAttribute(
            GetStdHandle(STD_OUTPUT_HANDLE),
            color
        );
    }

    inline void reset() {
        set(DEFAULT);
    }
}
