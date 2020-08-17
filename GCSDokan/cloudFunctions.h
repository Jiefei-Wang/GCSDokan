#pragma once
#include <string>
#include <vector>
#include <map>
#include <chrono>

struct file_meta_info {
    std::wstring remote_full_path;
    std::wstring local_name;
    std::wstring remote_name;
    size_t size = 0;
    std::wstring crc32c;
    std::chrono::system_clock::time_point time_created;
    std::chrono::system_clock::time_point time_updated;
    std::chrono::system_clock::time_point time_accessed;
};


struct folder_meta_info {
    //This attribute is not quite useful
    std::wstring local_name;
    //Just folder names, it is not a path
    std::vector<std::wstring> folder_names;
    //File local name, not file path
    std::map<std::wstring, file_meta_info> file_meta_vector;
};


folder_meta_info gcs_get_folder_meta(std::wstring linux_path);
//file_meta_info gcs_get_file_meta(std::wstring win_path);

bool gcs_read_file(std::wstring win_path, void* Buffer,
    size_t offset,
    size_t length);
