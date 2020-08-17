#pragma once
#include <string>
#include "cloudFunctions.h"
#include "dokan/dokan.h"
#include "globalVariables.h"


extern std::wstring remote_link;

struct file_manager_handle_struct {
    CACHE_TYPE cache_type = CACHE_TYPE::none;
    std::wstring win_path;
    std::wstring linux_full_path;
    void* cache_info = nullptr;
};
typedef file_manager_handle_struct* file_manager_handle;


bool exist_path(std::wstring win_path);
bool exist_file(std::wstring win_path);
bool exist_folder(std::wstring win_path);
folder_meta_info get_folder_meta(std::wstring win_path);
file_meta_info get_file_meta(std::wstring win_path);


file_manager_handle get_file_manager_handle(std::wstring win_path);
void release_file_manager_handle(file_manager_handle manager_handle);
// read_file do not check the existance of the file
NTSTATUS get_file_data(file_manager_handle file_manager, void* buffer, size_t offset, size_t length);


void set_remote_link(std::wstring link);
void set_cache_root(std::wstring path);
void set_cache_type(CACHE_TYPE type);
void set_refresh_interval(size_t time);

std::wstring get_remote_link();
std::wstring get_cache_root();
CACHE_TYPE get_cache_type();
size_t get_refresh_interval();

void set_mount_point(std::string path);
void set_remote_link(std::string link);
void set_cache_root(std::string path);
