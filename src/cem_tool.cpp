#include <regex>
#include <iostream>
#include <cstdio>
#include <ctime>
#include <fstream>

#include "cem_tool.hpp"
#include "zip_archive.hpp"
#include "string_helper.hpp"



cem_tool::cem_tool(const std::vector<std::string>& args) {
    if(args.size() != 2) {
        std::printf("%s", usage);
        exit(0);
    }

    // Make sure provided file path is valid.
    ext_zip_filepath = std::filesystem::absolute(args[1]);

    if(!std::filesystem::exists(ext_zip_filepath)) {
        std::fprintf(stderr, "File dosent exist.\n%s", usage);
        exit(-1);
    }

    if(!std::filesystem::is_regular_file(ext_zip_filepath)) {
        std::fprintf(stderr, "Not a file.\n%s", usage);
        exit(-1);
    }

    if(ext_zip_filepath.extension() != ".zip") {
        std::fprintf(stderr, "Not a zip file.\n%s", usage);
        exit(-1);
    }
}


int cem_tool::run() {
    fusion::cem_ext_manifest ext_man = {};
    std::filesystem::path editor_mfx;           // Used to get mfx name and gets loaded later.

    if(std::filesystem::exists("./temp")) {
        std::printf("Removing './temp' directory...\n");
        std::filesystem::remove_all("./temp");          // TODO: Maybe ask user before deleting something? Add a flag to remove it without asking?
    }

    // Open the zip file, get all info we can and extract it in 'temp' directory.
    try {
        zip_archive ext_zip;

        ext_zip.open(ext_zip_filepath);

        std::vector<std::filesystem::path> files = ext_zip.list_files();
        zip_file_sanity_check(ext_zip_filepath.stem().string(), files);

        editor_mfx = find_editor_mfx(files);

        // Latest modified date
        guess_mfx_name(&ext_man, editor_mfx);
        guess_supported_platforms(&ext_man, files);

        for (auto &&e : ext_zip.get_file_entries()) {
            if(ext_man.time < e.modified_date) {
                ext_man.time = e.modified_date;
            }
        }

        for (auto &&f : files) {
            ext_man.files.push_back(f.string());
        }

        ext_zip.extract("temp");
    }
    catch(const std::exception& e) {
        std::fprintf(stderr, "%s\n", e.what());
        return -1;
    }

    ext_man.download = ext_man.mfxname;
    ext_man.zipsize = std::filesystem::file_size(ext_zip_filepath);


    // Try to load the editor mfx and get more infos.
    try {
        fusion::extension ext;
        ext.open("./temp" / editor_mfx);

        ext.Initialize(1);

        ext_man.dev = ext.GetInfos(fusion::ext_general_infos::product) == 3;    // 3 = Developer, 2 = Standard, 1 = TGF.

        fusion::ext_infos infos = {};
        ext.GetObjInfos(&infos);
        ext_man.name = infos.name;
        ext_man.author = infos.author;
        ext_man.description = infos.comment;
        ext_man.website = infos.website;

        ext.Free();
    }
    catch(const std::exception& e) {
        std::fprintf(stderr, "%s\n", e.what());
        return -1;
    }


    std::string output_filename = ext_man.mfxname + ".json";
    std::ofstream output(output_filename);
    output << ext_man.to_json();
    std::printf("Created '%s', make sure the file is correct.\n", output_filename.c_str());
    return 0;
}

cem_tool::~cem_tool() {
    std::fflush(stdout);
    std::fflush(stderr);
}



// Check the zip file structure, make sure:
// - Ext file names are the same
// - Directory structure is correct
// - Required ext files are present
void cem_tool::zip_file_sanity_check(const std::string& ext_name, const std::vector<std::filesystem::path>& zip_files) {
    // Test all extension runtime and editor files if they have consistent names.
    // Not realy possible to combine those because capture groups get messed up.
    const char* name_tests[] = {
        "Extensions/(?:Unicode/|HWA/)?(.*)\\.mfx",
        "Data/Runtime/(?:Unicode/|HWA/)?(.*)\\.mfx",
        "Data/Runtime/Flash/(.*)\\.zip",
        "Data/Runtime/Android/(.*)\\.zip",
        "Data/Runtime/iPhone/(.*)\\.ext",
        "Data/Runtime/Html5/(.*)\\.js",
        "Data/Runtime/Wua/js/runtime/extensions/source/(.*)\\.js",
        "Data/Runtime/Mac/(.*)\\.dat",
        "Data/Runtime/XNA/(?:Phone|Windows|Xbox)/(.*)\\.zip",
    };

    for (auto &&t : name_tests) {
        std::smatch match;
        std::regex test(t);

        for (auto &&f : zip_files) {
            auto filepath = f.string();

            if(std::regex_search(filepath, match, test) && match[1] != ext_name) {
                throw create_except("Bad zip file structure: File '%s' is named '%s' but expected '%s'.", filepath.c_str(), match[1].str().c_str(), ext_name.c_str());
            }
        }
    }

    // Check directories in zip file, all must be matched by that massive regex bellow.
    // Create a copy of the path so the actual array contents stay the same.
    for (auto f : zip_files) {
        auto directory = f.remove_filename().string();
        bool match = false;

        // ...
        std::regex test("Extensions/(Unicode/|HWA/)?|Data/Runtime/((Unicode/|HWA/)?|Flash/|Android/|iPhone/|Html5/|Wua/js/runtime/extensions/source/|Mac/|XNA/(Phone|Windows|Xbox)/)|Examples/(.*)?|Help/(.*)?");
        if(!std::regex_match(directory, test)) {
            throw create_except("Bad zip file structure: Directory '%s' was not recognized, typo?", directory.c_str());
        }
    }

    // Check if any editor .mfx is present in Extensions/
    bool match = false;

    std::regex editor_mfx_regex("Extensions/(Unicode/|HWA/)?.*\\.mfx");
    for (auto &&f : zip_files) {
        auto filepath = f.string();
        if(std::regex_match(filepath, editor_mfx_regex)) {
            match = true;
            break;
        }
    }

    if(!match) {
        throw std::exception("Bad zip file structure: The zip file doesnt contain any editor .mfx file.");
    }

    // Check if at least one runtime extension file is present in Data/Runtime/
    match = false;

    std::regex runtime_mfx_regex("Data/Runtime/((Unicode/|HWA/)?.*\\.mfx|Flash/.*\\.zip|Android/.*\\.zip|iPhone/.*\\.ext|Html5/.*\\.js|Wua/js/runtime/extensions/source/.*\\.js|Mac/.*\\.dat|XNA/(Phone|Windows|Xbox)/.*\\.zip)");
    for (auto &&f : zip_files) {
        auto filepath = f.string();
        if(std::regex_match(filepath, runtime_mfx_regex)) {
            match = true;
            break;
        }
    }

    if(!match) {
        throw std::exception("Bad zip file structure: The zip file doesnt contain any runtime extension file.");
    }
}


void cem_tool::guess_mfx_name(fusion::cem_ext_manifest* ext_man, const std::filesystem::path& editor_mfx_path) {
    std::regex mfx_name_regex(".*/(.*)\\.mfx");         // Get mfxname from editor .mfx
    std::smatch match;

    auto filepath = editor_mfx_path.string();

    if(std::regex_search(filepath, match, mfx_name_regex)) {
        ext_man->mfxname = match[1];
    }
}

void cem_tool::guess_supported_platforms(fusion::cem_ext_manifest* ext_man, const std::vector<std::filesystem::path>& zip_files) {
    // Matches fusion::platform enum
    const char* tests[] = {
        "Data/Runtime/(Unicode/|HWA/)?.*\\.mfx",
        "Data/Runtime/Flash/.*\\.zip",
        "Data/Runtime/Android/.*\\.zip",
        "Data/Runtime/iPhone/.*\\.ext",
        "Data/Runtime/Html5/.*\\.js",
        "Data/Runtime/Wua/js/runtime/extensions/source/.*\\.js",
        "Data/Runtime/Mac/.*\\.dat",
        "Data/Runtime/XNA/(Phone|Windows|Xbox)/.*\\.zip",
    };

    std::uint32_t supported_platforms = 0;

    for (size_t i = 0; i < 8; i++) {
        std::regex regex(tests[i]);

        for (auto &&f : zip_files) {
            auto filepath = f.string();

            if(std::regex_match(filepath, regex)) {
                supported_platforms |= fusion::platform_index_to_enum(i);
                break;
            }
        }
    }

    if(!supported_platforms) {
        throw std::exception("No platforms supported? Bad file structure?");
    }

    ext_man->platforms = supported_platforms;
}


std::filesystem::path cem_tool::find_editor_mfx(const std::vector<std::filesystem::path>& zip_files) {
    std::regex editor_mfx_regex("Extensions/(Unicode/|HWA/)?.*\\.mfx");

    for (auto &&f : zip_files) {
        auto filepath = f.string();

        if(std::regex_match(filepath, editor_mfx_regex)) {
            return filepath;
        }
    }

    throw std::exception("No editor .mfx file? Bad file structure?");
}