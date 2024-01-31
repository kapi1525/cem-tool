#pragma once

#include <filesystem>
#include <vector>
#include <string>

#include "fusion_ext.hpp"



class cem_tool {
public:
    cem_tool() = delete;
    cem_tool(const std::vector<std::string>& args);
    ~cem_tool();

    int run();

private:
    const char* usage = "Usage: cem-tool [zip file]\n";

    std::filesystem::path ext_zip_filepath;

    void zip_file_sanity_check(const std::string& ext_name, const std::vector<std::filesystem::path>& zip_files);

    void guess_mfx_name(fusion::cem_ext_manifest* ext_man, const std::filesystem::path& editor_mfx_path);
    void guess_supported_platforms(fusion::cem_ext_manifest* ext_man, const std::vector<std::filesystem::path>& zip_files);

    std::filesystem::path find_editor_mfx(const std::vector<std::filesystem::path>& zip_files);
};