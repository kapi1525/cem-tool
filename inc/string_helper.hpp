#pragma once

#include <string>
#include <string_view>
#include <stdexcept>
#include <cinttypes>
#include <cstdarg>



std::string to_utf8(std::wstring_view utf16_str);
std::wstring to_utf16(std::string_view utf8_str);


// Helper function to create fancy exceptions
template <class T = std::exception>
T create_except(const char* fmt...) {
    std::va_list args;
    va_start(args, fmt);

    int buf_size = std::vsnprintf(nullptr, 0, fmt, args) + 1;   // vsnprintf doesnt include null terminator
    char* buffer = new char[buf_size];

    std::vsnprintf(buffer, buf_size, fmt, args);
    va_end(args);

    auto ret = T(buffer);
    delete buffer;

    return ret;
}


class windows_utf8_in_console {
public:
    windows_utf8_in_console();
    ~windows_utf8_in_console();

private:
    std::uint32_t before_codepage, before_out_codepage;
};