#include "cem_tool.hpp"
#include "string_helper.hpp"

#include <vector>
#include <string>
#include <iostream>



int wmain(int argc, const wchar_t* argv[]) {
    windows_utf8_in_console utf8;   // unicode madness

    std::vector<std::string> args;

    for (size_t i = 0; i < argc; i++) {
        args.push_back(to_utf8(argv[i]));
    }

    cem_tool app(args);
    return app.run();
}
