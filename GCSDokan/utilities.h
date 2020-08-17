#pragma once
#include <string>
#include <chrono>
#include <vector>
#include "dokan/dokan.h"

#ifdef _DEBUG
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#else
#define DBG_NEW new
#endif


struct decomposed_path {
    std::string bucket;
    std::string name;
    std::string path;
    std::string parent_path;
    std::vector<std::string> full_path_vec;
};

//CMD related function
std::string execute_command(const char* cmd);
bool is_command_exist(const char* command);


std::wstring to_linux_slash(std::wstring path);
std::wstring  to_win_slash(std::wstring path);
std::wstring to_parent_folder(std::wstring path);
std::string to_parent_folder(std::string path);
std::wstring get_file_name_in_path(std::wstring path);
std::string get_tailing_string(std::string source, size_t offset);
std::wstring remove_tailing_slash(std::wstring path);
std::string remove_tailing_slash(std::string path);
decomposed_path get_path_info(std::wstring win_path);


/*
Examples:
build_path(L"", L"bucket") = L"bucket"
build_path(L"bucket", L"") = L"bucket"
build_path(L"bucket", L"/") = L"bucket/"
build_path(L"bucket", L"test") = L"bucket/test"
build_path(L"bucket/", L"test") = L"bucket/test"
build_path(L"bucket/", L"/test") = L"bucket/test"
*/
std::string build_path(std::string path1, std::string path2, char delimiter = '/');
std::wstring build_path(std::wstring path1, std::wstring path2, WCHAR delimiter = L'/');

void debug_print(LPCWSTR format, ...);
void error_print(LPCWSTR format, ...);
void debug_print(LPCSTR format, ...);
void error_print(LPCSTR format, ...);
std::wstring stringToWstring(const char* utf8Bytes);
std::string wstringToString(const WCHAR* utf16Bytes);
std::wstring stringToWstring(std::string utf8Bytes);
std::string wstringToString(std::wstring utf16Bytes);
FILETIME time_point_to_file_time(std::chrono::system_clock::time_point time);


bool endsWith(std::wstring str, std::wstring suffix);

//Splint a long long type variable to its low and high order parts
static inline void LlongToDwLowHigh(const LONGLONG& value, DWORD& low,
    DWORD& hight) {
    hight = value >> 32;
    low = static_cast<DWORD>(value);
}


std::string to_base64(void* data, size_t size);