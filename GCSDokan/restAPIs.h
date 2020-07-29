#pragma once
#include <string>
#include <vector>
#include "dataTypes.h"



folder_meta_info gcs_get_folder_meta(std::wstring win_path);
//file_meta_info gcs_get_file_meta(std::wstring win_path);

void gcs_read_file(std::wstring win_path, void* Buffer,
    size_t& buffer_length,
    size_t offset);
