#pragma once
#include <vector>
#include <map>
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
    std::wstring local_full_path;
    std::wstring remote_full_path;
    std::wstring local_name;
    std::wstring remote_name;
    size_t size = 0;
    std::wstring crc32c;
    FILETIME time_created;
    FILETIME time_updated;
    FILETIME time_accessed;
} file_meta_info;


typedef struct {
    std::wstring local_name;
    std::wstring local_full_path;
    std::vector<std::wstring> folder_names;
    std::map<std::wstring,file_meta_info> file_meta_vector;
} folder_meta_info;


#include <ctime>   
typedef struct {
    std::chrono::system_clock::time_point query_time;
    bool exist = false;
    folder_meta_info folder_meta;
} REST_result_holder;