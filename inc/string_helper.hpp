#pragma once

#include <string>
#include <string_view>
#include <stdexcept>
#include <cinttypes>



std::string to_utf8(std::wstring_view utf16_str);
std::wstring to_utf16(std::string_view utf8_str);


class windows_utf8_in_console {
public:
    windows_utf8_in_console();
    ~windows_utf8_in_console();

private:
    std::uint32_t before_codepage, before_out_codepage;
};