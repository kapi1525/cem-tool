#pragma once

#include <filesystem>

// Fancy minizip abstraction



struct zip_archive_entry {
    std::filesystem::path filepath;
    std::time_t modified_date;
};


class zip_archive {
public:
    zip_archive() = default;
    ~zip_archive();

    void open(std::filesystem::path file_path);
    void close();
    void extract(std::filesystem::path extract_path);

    bool is_open();

    std::vector<zip_archive_entry> get_entries();
    std::vector<zip_archive_entry> get_file_entries();
    std::vector<std::filesystem::path> list_files();

private:
    // zero init
    void* zip_handle = 0;
};