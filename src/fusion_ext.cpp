#include <cinttypes>
#include <filesystem>
#include <exception>
#include <iostream>
#include "fusion_ext.hpp"
#include "string_helper.hpp"

#include "nlohmann/json.hpp"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>



std::uint32_t fusion::platform_enum_to_index(platform platform_enum) {
    // There is probably a fancier way to do this
    std::uint32_t index = 0;
    for (std::uint32_t i = 1; i < platform::last; i=i<<1) {
        if(platform_enum == i) {
            return index;
        }
        index++;
    }
    return 0;
}

fusion::platform fusion::platform_index_to_enum(std::uint32_t platform_index) {
    return static_cast<platform>(1 << platform_index);
}



static std::string yes_no(bool b) {
    if(b) {
        return "yes";
    }
    return "no";
}

static nlohmann::json files_array(fusion::cem_ext_manifest* ext) {
    nlohmann::json files;

    for (auto&& i : ext->files) {
        files.push_back(i);
    }

    return files;
}

static std::string supported_platforms(fusion::cem_ext_manifest* ext) {
    uint32_t p = ext->platforms;

    std::string ret = "";

    size_t name_array_index = 0;
    for (uint32_t i = 1; i < fusion::platform::last; i=i<<1) {
        if(p & i) {
            if(!ret.empty() && ret.back() != ',') {
                ret.append(",");
            }
            ret.append(fusion::platform_names[name_array_index]);
            // std::printf("%d", i);
        }
        name_array_index++;
    }

    return ret;
}

static std::string last_modification_time(fusion::cem_ext_manifest* ext) {
    char buf[sizeof("YYYY.MM.DD:HH.MM.SS")];

    tm local;
    localtime_s(&local, &ext->time);

    // magic
    // Original ZipToJson tool ignores DST so try to ignore it as well... This maybe works?
    if(local.tm_isdst) {
        local.tm_hour--;
        if(local.tm_hour == -1) {
            local.tm_hour = 23;
        }
    }

    std::strftime(buf, sizeof(buf), "%Y.%m.%d:%H.%M.%S", &local);
    // std::printf("local %s %i\n", buf, local->tm_isdst);

    return buf;
}


std::string fusion::cem_ext_manifest::to_json() {
    nlohmann::ordered_json j = {
        {"mfxname", mfxname},
        {"name", name},
        {"author", author},
        {"description", description},
        {"website", website},
        {"dev", yes_no(dev)},
        {"platforms", supported_platforms(this)},
        {"time", last_modification_time(this)},
        {"download", download},
        {"zipsize", std::to_string(zipsize)},
        {"files", files_array(this)}
    };

    return j.dump(4);   // 4 Spaces
}



void fusion::extension::open(std::filesystem::path mfx_path) {
    auto mfx_path_str = std::filesystem::absolute(mfx_path).string();
    module_handle = LoadLibraryExW(to_utf16(mfx_path_str).c_str(), NULL, LOAD_LIBRARY_SEARCH_APPLICATION_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);

    if(module_handle == nullptr) {
        throw create_except("Failed to load extension '%s': %s.", mfx_path_str.c_str(), last_system_error().c_str());
    }

    namespace f = fusion::api::ext_funcs;
    funcs.Initialize = (f::Initialize*)get_proc(module_handle, "Initialize");
    funcs.Free = (f::Free*)get_proc(module_handle, "Free");
    funcs.GetInfos = (f::GetInfos*)get_proc(module_handle, "GetInfos");
    funcs.GetRunObjectInfos = (f::GetRunObjectInfos*)get_proc(module_handle, "GetRunObjectInfos");
    funcs.GetObjInfosA = (f::GetObjInfosA*)get_proc(module_handle, "GetObjInfos");
    funcs.GetObjInfosW = (f::GetObjInfosW*)get_proc(module_handle, "GetObjInfos");

    // DarkEdif uses GetVersion in Initialize()
    dummy_mv.GetVersion = []() -> std::uint32_t { return 0; };

    is_unicode = GetInfos(ext_general_infos::unicode);
}

void fusion::extension::close() {
    if(!FreeLibrary((HMODULE)module_handle)) {
        throw create_except("FreeLibrary failed: %s.", last_system_error().c_str());
    }

    module_handle = 0;
}


bool fusion::extension::is_open() {
    return module_handle != 0;
}


short fusion::extension::Initialize(int quiet) {
    return funcs.Initialize(&dummy_mv, quiet);
}

int fusion::extension::Free() {
    return funcs.Free(&dummy_mv);
}


std::uint32_t fusion::extension::GetInfos(ext_general_infos info) {
    return funcs.GetInfos((int)info);
}

short fusion::extension::GetRunObjectInfos(ext_run_infos* infos_ptr) {
    return funcs.GetRunObjectInfos(&dummy_mv, infos_ptr);
}

void fusion::extension::GetObjInfos(ext_infos* infos_ptr) {
    if(is_unicode) {
        wchar_t name[MAX_PATH];
        wchar_t author[MAX_PATH];
        wchar_t copyright[MAX_PATH];
        wchar_t comment[MAX_PATH];
        wchar_t website[MAX_PATH];

        funcs.GetObjInfosW(&dummy_mv, (void*)nullptr, name, author, copyright, comment, website);

        infos_ptr->name = to_utf8(name);
        infos_ptr->author = to_utf8(author);
        infos_ptr->copyright = to_utf8(copyright);
        infos_ptr->comment = to_utf8(comment);
        infos_ptr->website = to_utf8(website);
    } else {
        char name[MAX_PATH];
        char author[MAX_PATH];
        char copyright[MAX_PATH];
        char comment[MAX_PATH];
        char website[MAX_PATH];

        funcs.GetObjInfosA(&dummy_mv, (void*)nullptr, name, author, copyright, comment, website);

        infos_ptr->name = name;
        infos_ptr->author = author;
        infos_ptr->copyright = copyright;
        infos_ptr->comment = comment;
        infos_ptr->website = website;
    }
}


void* fusion::extension::get_proc(void* handle, const std::string& proc) {
    auto ret = GetProcAddress((HMODULE)handle, proc.c_str());

    if(!ret) {
        throw create_except("Failed to get funcion '%s': %s.", proc.c_str(), last_system_error().c_str());
    }

    return ret;
}