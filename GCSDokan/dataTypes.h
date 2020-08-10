#pragma once
#include <vector>
#include <map>
#include <string>
#include <chrono>
#include "dokan/dokan.h"
/*
This struct contains the information of a decomposed path
bucket: the name of the bucket
path: the relative path inside the bucket
full_path_vec: the hierarchical path components(including bucket name)
*/
struct decomposed_path {
    std::wstring bucket;
    std::wstring name;
    std::wstring path;
    std::wstring parent_path;
    std::vector<std::wstring> full_path_vec;
};


struct file_meta_info {
    std::wstring local_full_path;
    std::wstring remote_full_path;
    std::wstring local_name;
    std::wstring remote_name;
    size_t size = 0;
    std::wstring crc32c;
    FILETIME time_created;
    FILETIME time_updated;
    FILETIME time_accessed;
};


struct folder_meta_info {
    std::wstring local_name;
    std::wstring local_full_path;
    std::vector<std::wstring> folder_names;
    //Key is file local name, not file path
    std::map<std::wstring,file_meta_info> file_meta_vector;
};


#include <ctime>   
struct REST_result_holder{
    std::chrono::system_clock::time_point query_time;
    bool exist = false;
    folder_meta_info folder_meta;
};





enum class CACHE_TYPE { none, disk, memory };

struct file_manager_handle_struct {
    CACHE_TYPE cache_type = CACHE_TYPE::none;
    std::wstring win_path;
    std::wstring linux_full_path;
    void* cache_info = nullptr;
};
typedef file_manager_handle_struct* file_manager_handle;


struct file_private_info {
    size_t event_id = 0;
    file_manager_handle manager_handle = nullptr;
};