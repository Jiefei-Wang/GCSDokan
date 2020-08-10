#pragma once
#include <string>
#include "dataTypes.h"
#include "dokan/dokan.h"


extern std::wstring remote_link;

bool exist_path(std::wstring win_path);

bool exist_file(std::wstring win_path);
bool exist_folder(std::wstring win_path);
folder_meta_info get_folder_meta(std::wstring win_path);
file_meta_info get_file_meta(std::wstring win_path);


file_manager_handle get_file_manager_handle(std::wstring win_path);
void release_file_manager_handle(file_manager_handle file_manager);
// read_file do not check the existance of the file
NTSTATUS get_file_data(file_manager_handle file_manager, void* buffer, size_t offset, size_t length);
