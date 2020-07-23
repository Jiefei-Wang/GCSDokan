#pragma once
#include <vector>
#include <string>
#include <chrono>
#include "dokan.h"
/*
This struct contains the information of a decomposed path
bucket: the name of the bucket
path: the relative path inside the bucket
full_path_vec: the hierarchical path components(including bucket name)
*/
typedef struct {
    std::wstring bucket;
    std::wstring path;
    std::wstring parent_path;
    std::vector<std::wstring> full_path_vec;
} decomposed_path;


typedef struct {
    std::wstring win_full_path;
    std::wstring cloud_full_path;
    std::wstring win_name;
    std::wstring cloud_name;
    size_t size = 0;
    std::wstring crc32c;
    FILETIME time_created;
    FILETIME time_updated;
} file_meta_info;


typedef struct {
    std::vector<std::wstring> folder_names;
    std::vector<file_meta_info> file_meta_vector;
} folder_meta_info;


#include <ctime>   
typedef struct {
    std::chrono::system_clock::time_point query_time;
    bool exist = false;
    bool is_file = false;
    folder_meta_info folder_meta;
    file_meta_info file_meta;
} REST_result_holder;