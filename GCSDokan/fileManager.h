#pragma once
#include <string>
#include "dataTypes.h"
#include "dokan.h"



extern std::wstring root_path;

bool exist_path(std::wstring win_path);

bool exist_file(std::wstring win_path);
bool exist_folder(std::wstring win_path);
folder_meta_info get_folder_meta(std::wstring win_path);
file_meta_info get_file_meta(std::wstring win_path);



// read_file do not check the existance of the file
NTSTATUS get_file_data(std::wstring win_path, void* buffer,
    size_t& buffer_len,
    size_t offset);
