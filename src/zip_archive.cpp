#include <cassert>

#include "zip_archive.hpp"

#include "mz.h"
#include "mz_zip.h"
#include "mz_strm.h"
#include "mz_zip_rw.h"




zip_archive::~zip_archive() {
    if(is_open()) {
        close();
    }
}


void zip_archive::open(std::filesystem::path file_path) {
    if(is_open()) {
        close();
    }

    zip_handle = mz_zip_reader_create();

    if(mz_zip_reader_open_file(zip_handle, file_path.string().c_str()) != MZ_OK) {
        throw std::exception("Failed to open zip file.");
    }
}

void zip_archive::close() {
    if(!is_open()) {
        return;
    }

    mz_zip_reader_close(zip_handle);
}


#ifndef MZ_ZIP_NO_DECOMPRESSION
void zip_archive::extract(std::filesystem::path extract_path) {
    if(!is_open()) {
        throw std::logic_error("Failed to extract zip file: File is not open.");
    }

    if(mz_zip_reader_save_all(zip_handle, extract_path.string().c_str()) != MZ_OK) {
        throw std::exception("Failed to extract zip file.");
    }
}
#else
void zip_archive::extract(std::filesystem::path extract_path) {
    throw std::logic_error("Failed to extract zip file: minizip was built with no decompression support.");
}
#endif


bool zip_archive::is_open() {
    return mz_zip_reader_is_open(zip_handle) == MZ_OK;
}



std::vector<zip_archive_entry> zip_archive::get_entries() {
    std::vector<zip_archive_entry> enties = {};

    if(!is_open()) {
        return enties;
    }

    mz_zip_file* file_info = nullptr;

    if (mz_zip_reader_goto_first_entry(zip_handle) == MZ_OK) {
        do {
            if (mz_zip_reader_entry_get_info(zip_handle, &file_info) != MZ_OK) {
                break;
            }

            enties.push_back({file_info->filename, file_info->modified_date});
        } while (mz_zip_reader_goto_next_entry(zip_handle) == MZ_OK);
    }

    return enties;
}

std::vector<zip_archive_entry> zip_archive::get_file_entries() {
    std::vector<zip_archive_entry> enties = {};

    if(!is_open()) {
        return enties;
    }

    mz_zip_file* file_info = nullptr;

    if (mz_zip_reader_goto_first_entry(zip_handle) == MZ_OK) {
        do {
            if (mz_zip_reader_entry_get_info(zip_handle, &file_info) != MZ_OK) {
                break;
            }

            if(mz_zip_reader_entry_is_dir(zip_handle) == MZ_OK) {
                continue;
            }

            enties.push_back({file_info->filename, file_info->modified_date});
        } while (mz_zip_reader_goto_next_entry(zip_handle) == MZ_OK);
    }

    return enties;
}

std::vector<std::filesystem::path> zip_archive::list_files() {
    std::vector<std::filesystem::path> files = {};

    if(!is_open()) {
        return files;
    }

    mz_zip_file* file_info = nullptr;

    if (mz_zip_reader_goto_first_entry(zip_handle) == MZ_OK) {
        do {
            if (mz_zip_reader_entry_get_info(zip_handle, &file_info) != MZ_OK) {
                break;
            }

            if(mz_zip_reader_entry_is_dir(zip_handle) == MZ_OK) {
                continue;
            }

            files.push_back(file_info->filename);
        } while (mz_zip_reader_goto_next_entry(zip_handle) == MZ_OK);
    }

    return files;
}