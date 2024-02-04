#include "string_helper.hpp"

#include <cstdio>           // std::snprintf
#include <cassert>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>



std::string to_utf8(std::wstring_view utf16_str) {
    int buffer_size = WideCharToMultiByte(
        CP_UTF8,
        0,
        utf16_str.data(),
        utf16_str.size(),
        nullptr,
        0,
        nullptr,
        nullptr
    );

    if(buffer_size <= 0) {
        return "";
    }

    char* buffer = new char[buffer_size];

    int status = WideCharToMultiByte(
        CP_UTF8,
        0,
        utf16_str.data(),
        utf16_str.size(),
        buffer,
        buffer_size,
        nullptr,
        nullptr
    );

    if(status == 0) {
        std::uint32_t err = GetLastError();
        const size_t error_buf_size = 128;
        char error_buf[error_buf_size];

        std::snprintf(error_buf, error_buf_size, "UTF16 to UTF8 text conversion failed: %i.", err);
        throw std::runtime_error(error_buf);
    }

    return std::string(buffer, buffer_size);
}

std::wstring to_utf16(std::string_view utf8_str) {
    int buffer_size = MultiByteToWideChar(
        CP_UTF8,
        0,
        utf8_str.data(),
        utf8_str.size(),
        nullptr,
        0
    );

    if(buffer_size <= 0) {
        return L"";
    }

    wchar_t* buffer = new wchar_t[buffer_size];

    int status = MultiByteToWideChar(
        CP_UTF8,
        0,
        utf8_str.data(),
        utf8_str.size(),
        buffer,
        buffer_size
    );

    if(status == 0) {
        std::uint32_t err = GetLastError();
        const size_t error_buf_size = 128;
        char error_buf[error_buf_size];

        std::snprintf(error_buf, error_buf_size, "UTF8 to UTF16 text conversion failed: %i.", err);
        throw std::runtime_error(error_buf);
    }

    return std::wstring(buffer, buffer_size);
}


std::string last_system_error() {
    const size_t buf_size = 256;
    wchar_t buf[buf_size];
    std::uint32_t last_error = GetLastError();

    std::uint32_t written = FormatMessageW(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        last_error,
        0,
        buf,
        buf_size,
        nullptr
    );

    if(written < 3) {
        return "Failed to format error message, good luck with debugging";
    }

    // written-3 to ignore "./r/n" at the end of error message.
    auto system_error_str = to_utf8(std::wstring_view(buf, written-3));

    const size_t ret_buf_size = 256;
    char ret_buf[ret_buf_size];

    std::snprintf(ret_buf, ret_buf_size, "(%u) %s", last_error, system_error_str.c_str());
    return ret_buf;
}



windows_utf8_in_console::windows_utf8_in_console() {
    before_codepage = GetConsoleCP();
    before_out_codepage = GetConsoleOutputCP();
    if(before_codepage && before_out_codepage) {
        SetConsoleCP(CP_UTF8);
        SetConsoleOutputCP(CP_UTF8);
    }
}

windows_utf8_in_console::~windows_utf8_in_console() {
    if(before_codepage && before_out_codepage) {
        SetConsoleCP(before_codepage);
        SetConsoleOutputCP(before_out_codepage);
    }
}
